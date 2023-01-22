#if (defined PMU_6555_V500) || (defined FASTBOOT_PMU_HI6555V500) || (defined CONFIG_HISI_HI6555V500_PMU)
#include "pmic_interface_55v500.h"
#else
#include "pmic_interface_55v200.h"
#endif
