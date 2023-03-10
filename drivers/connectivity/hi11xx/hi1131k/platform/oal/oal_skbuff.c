

#include "arch/oal_skbuff.h"
#include "oal_ext_if.h"
oal_uint32 g_ul_skb_data_alloc_fail_count = 0;

struct sk_buff *__alloc_skb(unsigned int size, gfp_t gfp_mask, int fclone, int node)
{
    struct sk_buff *skb;
    oal_uint8 *data = OAL_PTR_NULL;
    skb = (struct sk_buff *)memalign(CACHE_ALIGNED_SIZE, SKB_DATA_ALIGN(sizeof(struct sk_buff)));
    if (skb == NULL) {
        printf("#########alloc_skb skb is NULL##########\n");
        return NULL;
    }
    data = (oal_uint8 *)memalign(CACHE_ALIGNED_SIZE, size);
    if (data == NULL) {
        printf("#########alloc_skb data is NULL##########\n");
        g_ul_skb_data_alloc_fail_count++;
        free(skb);
        return NULL;
    }

    if (skb == NULL)
        return NULL;
    memset_s(skb, sizeof(struct sk_buff), 0, sizeof(struct sk_buff));
    memset_s(data, size, 0, size);
    skb->truesize = SKB_TRUESIZE(size);
    oal_atomic_set(&skb->users, 1);
    skb->head = data;
    skb->data = data;
    skb_reset_tail_pointer(skb);
    skb->end = skb->tail + size;

    return skb;
}

struct sk_buff *alloc_skb(unsigned int size, gfp_t priority)
{
    return __alloc_skb(size, priority, 0, NUMA_NO_NODE);
}


struct sk_buff *dev_alloc_skb(unsigned int length)
{
    /*
     * There is more code here than it seems:
     * __dev_alloc_skb is an inline
     */
    return __dev_alloc_skb(length, GFP_ATOMIC);
}

static inline int skb_alloc_rx_flag(const struct sk_buff *skb)
{
    if (skb_pfmemalloc(skb))
        return SKB_ALLOC_RX;
    return 0;
}


unsigned char *skb_put(struct sk_buff *skb, unsigned int len)
{
    unsigned char *tmp = skb_tail_pointer(skb);
    skb->tail += len;
    skb->len  += len;
    if (skb->tail > skb->end)
        dprintf("!!skb_put overflow!!\n");
    return tmp;
}

struct sk_buff *skb_dequeue(struct sk_buff_head *list)
{
    unsigned long flags;
    struct sk_buff *result = OAL_PTR_NULL;

    spin_lock_irqsave(&list->lock, flags);
    result = __skb_dequeue(list);
    spin_unlock_irqrestore(&list->lock, flags);
    return result;
}

struct sk_buff *skb_dequeue_tail(struct sk_buff_head *list)
{
    unsigned long flags;
    struct sk_buff *result = OAL_PTR_NULL;

    spin_lock_irqsave(&list->lock, flags);
    result = __skb_dequeue_tail(list);
    spin_unlock_irqrestore(&list->lock, flags);
    return result;
}


void kfree_skb(struct sk_buff *skb)
{
    if (skb == NULL)
        return;
    free(skb->head);
    free(skb);
}

void skb_queue_purge(struct sk_buff_head *list)
{
    struct sk_buff *skb;
    while ((skb = skb_dequeue(list)) != NULL)
        kfree_skb(skb);
}

static void __copy_skb_header(struct sk_buff *new, const struct sk_buff *old)
{
    /* We do not copy old->sk */
    new->dev        = old->dev;
    memcpy_s(new->cb, sizeof(new->cb), old->cb, sizeof(old->cb));

    /* Note : this field could be in headers_start/headers_end section
     * It is not yet because we do not want to have a 16 bit hole
     */
    new->queue_mapping = old->queue_mapping;
}

static struct sk_buff *__skb_clone(struct sk_buff *n, struct sk_buff *skb)
{
#define C(x) n->x = skb->x

    n->next = n->prev = NULL;

    __copy_skb_header(n, skb);

    C(len);
    C(data_len);

    C(tail);
    C(end);
    C(head);
    C(data);
    C(truesize);
    oal_atomic_set(&n->users, 1);

    return n;
#undef C
}

struct sk_buff *skb_clone(struct sk_buff *skb, gfp_t gfp_mask)
{
    struct sk_buff *n;

    n = (struct sk_buff *)memalign(CACHE_ALIGNED_SIZE, SKB_DATA_ALIGN(sizeof(struct sk_buff)));
    if (n == NULL)
        return NULL;

    memset_s(n, SKB_DATA_ALIGN(sizeof(struct sk_buff)), 0, sizeof(struct sk_buff));

    return __skb_clone(n, skb);
}

static void copy_skb_header(struct sk_buff *new, const struct sk_buff *old)
{
    __copy_skb_header(new, old);
}

struct sk_buff *skb_copy(const struct sk_buff *skb, gfp_t gfp_mask)
{
    int headerlen = skb_headroom(skb);
    unsigned int size = skb_end_offset(skb) + skb->data_len;
    struct sk_buff *n = __alloc_skb(size, gfp_mask, skb_alloc_rx_flag(skb), NUMA_NO_NODE);

    if (n == NULL)
        return NULL;

    /* Set the data pointer */
    skb_reserve(n, headerlen);
    /* Set the tail pointer and length */
    skb_put(n, skb->len);

    if (skb_copy_bits(skb, -headerlen, n->head, headerlen + skb->len))
        BUG();

    copy_skb_header(n, skb);
    return n;
}

struct sk_buff *skb_copy_expand(const struct sk_buff *skb,
                                int newheadroom, int newtailroom, gfp_t gfp_mask)
{
    /*
     *  Allocate the copy buffer
     */
    struct sk_buff *n = __alloc_skb(newheadroom + skb->len + newtailroom,
                                    gfp_mask, skb_alloc_rx_flag(skb), NUMA_NO_NODE);
    int oldheadroom = skb_headroom(skb);
    int head_copy_len, head_copy_off;

    if (n == NULL)
        return NULL;

    skb_reserve(n, newheadroom);

    /* Set the tail pointer and length */
    skb_put(n, skb->len);

    head_copy_len = oldheadroom;
    head_copy_off = 0;
    if (newheadroom <= head_copy_len) {
        head_copy_len = newheadroom;
    } else {
        head_copy_off = newheadroom - head_copy_len;
    }
    /* Copy the linear header and data. */
    if (skb_copy_bits(skb, -head_copy_len, n->head + head_copy_off, skb->len + head_copy_len)) {
        BUG();
    }

    copy_skb_header(n, skb);

    return n;
}

void skb_queue_tail(struct sk_buff_head *list, struct sk_buff *newsk)
{
    unsigned long flags;

    spin_lock_irqsave(&list->lock, flags);
    __skb_queue_tail(list, newsk);
    spin_unlock_irqrestore(&list->lock, flags);
}

void skb_unlink(struct sk_buff *skb, struct sk_buff_head *list)
{
    unsigned long flags;

    spin_lock_irqsave(&list->lock, flags);
    __skb_unlink(skb, list);
    spin_unlock_irqrestore(&list->lock, flags);
}

unsigned char *skb_pull(struct sk_buff *skb, unsigned int len)
{
    return skb_pull_inline(skb, len);
}

void skb_trim(struct sk_buff *skb, unsigned int len)
{
    if (skb->len > len)
        __skb_trim(skb, len);
}

int skb_copy_bits(const struct sk_buff *skb, int offset, unsigned char *to, int len)
{
    int start = skb_headlen(skb);
    struct sk_buff *frag_iter = OAL_PTR_NULL;
    int i, copy;

    if (offset > (int)skb->len - len) {goto fault;}

    /* Copy header. */
    if ((copy = start - offset) > 0) {
        if (copy > len) {
            copy = len;
        }
        skb_copy_from_linear_data_offset(skb, offset, to, copy);
        if ((len -= copy) == 0) {return 0;}
        offset += copy;
        to     += copy;
    }

    if (!len) {return 0;}

fault:
    return -EFAULT;
}

int pskb_expand_head(struct sk_buff *skb, int nhead, int ntail, gfp_t gfp_mask)
{
    oal_uint8 *data = OAL_PTR_NULL;
    int size = nhead + (skb_end_pointer(skb) - skb->head) + ntail;
    long off;
    long data_off;

    BUG_ON(nhead < 0);

    size = SKB_DATA_ALIGN(size);

    data = (oal_uint8 *)memalign(CACHE_ALIGNED_SIZE, size);
    if (data == NULL)
    goto nodata;

    memset_s(data, size, 0, size);
    if (memcpy_s(data + nhead, size - nhead, skb->head, skb_tail_pointer(skb) - skb->head) != EOK) {
        OAL_IO_PRINT("pskb_expand_head::memcpy_s failed !");
    }

    data_off = skb->data - skb->head;

    kfree(skb->head);
    skb->head = data;
    skb->data = skb->head + data_off + nhead;
    skb->end = size;
    off = nhead;
    skb->tail += off;

    return 0;

nodata:
    return -ENOMEM;
}

void dev_kfree_skb(struct sk_buff *skb)
{
    if (unlikely(skb == NULL)) {return;}
    kfree_skb(skb);
}
void skb_queue_head(struct sk_buff_head *list, struct sk_buff *newsk)
{
    unsigned long flags;

    spin_lock_irqsave(&list->lock, flags);
    __skb_queue_head(list, newsk);
    spin_unlock_irqrestore(&list->lock, flags);
}
struct sk_buff *skb_unshare(struct sk_buff *skb, gfp_t pri)
{
#if 0
    might_sleep_if(pri & __GFP_WAIT);
    if (skb_cloned(skb)) {
        struct sk_buff *nskb = skb_copy(skb, pri);

        /* Free our shared copy */
        if (likely(nskb))
            consume_skb(skb);
        else
            kfree_skb(skb);
        skb = nskb;
    }
#endif
    return skb;
}

unsigned char *__pskb_pull_tail(struct sk_buff *skb, int delta)
{
    int i;
    int k;
    int eat = (skb->tail + delta) - skb->end;

    if (eat > 0) {
            return NULL;
    }

    if (skb_copy_bits(skb, skb_headlen(skb), skb_tail_pointer(skb), delta)) {
        BUG();
    }
    /* Estimate size of pulled pages. */
    /* If we need update frag list, we are in troubles. */
    skb->tail     += delta;
    skb->data_len -= delta;

    return skb_tail_pointer(skb);
}

int pskb_may_pull(struct sk_buff *skb, unsigned int len)
{
    if (likely(len <= skb_headlen(skb))) {return 1;}
    if (unlikely(len > skb->len)) {return 0;}
    return __pskb_pull_tail(skb, len - skb_headlen(skb)) != NULL;
}

