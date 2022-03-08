#ifndef __KSM
#define __KSM_H_

#include <stdlib.h>

#define KSM_FSCAN       "/sys/kernel/mm/ksm/full_scans"
#define KSM_MAX_PSH     "/sys/kernel/mm/ksm/max_page_sharing"
#define KSM_MERGE       "/sys/kernel/mm/ksm/merge_across_nodes"
#define KSM_PSHARED     "/sys/kernel/mm/ksm/pages_shared"
#define KSM_PSHARING    "/sys/kernel/mm/ksm/pages_sharing"
#define KSM_P2S         "/sys/kernel/mm/ksm/pages_to_scan"
#define KSM_PU          "/sys/kernel/mm/ksm/pages_unshared"
#define KSM_PV          "/sys/kernel/mm/ksm/pages_volatile"
#define KSM_RUN         "/sys/kernel/mm/ksm/run"
#define KSM_SLEEP_MS    "/sys/kernel/mm/ksm/sleep_millisecs"
#define KSM_SNC         "/sys/kernel/mm/ksm/stable_node_chains"
#define KSM_SNC_PMS     "/sys/kernel/mm/ksm/stable_node_chains_prune_millisecs"
#define KSM_SNDUPS      "/sys/kernel/mm/ksm/stable_node_dups"
#define KSM_USE_ZP      "/sys/kernel/mm/ksm/use_zero_pages"

uint ksm_init();
uint ksm_shared_pages();


#endif