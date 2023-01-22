/**
 * defrag.c
 *
 * Copyright (c) 2015 Jaegeuk Kim <jaegeuk@kernel.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "fsck.h"

static int migrate_block(struct f2fs_sb_info *sbi, u64 from, u64 to,
				struct f2fs_summary *sum)
{
	void *raw = calloc(BLOCK_SZ, 1);
	struct seg_entry *se;
	u64 offset;
	int ret, type;

	ASSERT(raw != NULL);

	/* read from */
	ret = dev_read_block(raw, from);
	ASSERT(ret >= 0);

	/* update sit bitmap & valid_blocks && se->type */
	se = get_seg_entry(sbi, GET_SEGNO(sbi, from));
	offset = OFFSET_IN_SEG(sbi, from);
	type = se->type;
	se->valid_blocks--;
	f2fs_clear_bit(offset, (char *)se->cur_valid_map);
	se->dirty = 1;

	se = get_seg_entry(sbi, GET_SEGNO(sbi, to));
	offset = OFFSET_IN_SEG(sbi, to);
	se->type = type;
	se->valid_blocks++;
	f2fs_set_bit(offset, (char *)se->cur_valid_map);
	se->dirty = 1;

	/* write to */
	set_write_stream_id(get_stream_id(type), to);
	ret = dev_write_block(raw, to);
	ASSERT(ret >= 0);

	/* if data block, read node and update node block */
	if (IS_DATASEG(type))
		update_data_blkaddr(sbi, le32_to_cpu(sum->nid),
				le16_to_cpu(sum->ofs_in_node), to);
	else
		update_nat_blkaddr(sbi, 0, le32_to_cpu(sum->nid), to);

	DBG(1, "Migrate %s block %"PRIx64" -> %"PRIx64"\n",
					IS_DATASEG(type) ? "data" : "node",
					from, to);
	free(raw);
	return 0;
}

int f2fs_defragment(struct f2fs_sb_info *sbi, u64 from, u64 len, u64 to, int left)
{
	struct seg_entry *se;
	u64 idx, offset;
	u64 count = 0;
	struct f2fs_summary sum;

	set_write_stream_id(META_STREAM_ID, sbi->sm_info->seg0_blkaddr);
	/* flush NAT/SIT journal entries */
	flush_journal_entries(sbi);
	ASSERT(f2fs_fsync_device() >= 0);

	for (idx = from; idx < from + len; idx++) {
		u64 target = to;

		se = get_seg_entry(sbi, GET_SEGNO(sbi, idx));
		offset = OFFSET_IN_SEG(sbi, idx);

		if (!f2fs_test_bit(offset, (const char *)se->cur_valid_map))
			continue;

		get_sum_entry(sbi, idx, &sum);
		if (find_next_free_block(sbi, &target, left, se->type, &sum)) {
			MSG(0, "Not enough space to migrate blocks");
			return -1;
		}

		if (migrate_block(sbi, idx, target, &sum)) {
			ASSERT_MSG("Found inconsistency: please run FSCK");
			return -1;
		}
		count++;
       }
	MSG(0, "Info: Totally migrate %"PRIu64" blocks\n", count); /*lint !e557*/

	zero_journal_entries(sbi);
	write_curseg_info(sbi);

	set_write_stream_id(META_STREAM_ID, sbi->sm_info->seg0_blkaddr);
	/* flush dirty sit entries */
	flush_sit_entries(sbi);

	write_checkpoints(sbi);

	return 0;
}