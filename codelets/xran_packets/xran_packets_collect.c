/*
*
* This codelet is used to collect data related to XRAN messages.
* Specifically it could the following
*  - for UL and DL it will collect the following user-plane info
*      - Total Packet count, total Prb count, amd inter-arrival histograms
*  - for DL it will collect the following ctrl-plane info
*      - Packet count, amd inter-arrival histograms
*
* The stat are collected in this codelet, but reported and zeroed in the associated reporing codelet.

* The inter-arrival histogram has 16 bins, with the bins representing the 
* following inter-arrival times (in nanosecs) ..
*   bin  range   
*    0:   0    .. 511
*    1:   512  .. 1,023 
*    2:   1,024 .. 2,047
*    3:   2,048 .. 4,095
*    4:   4,096 .. 8,191
*    5:   8,192 .. 16,383
*    6:   16,384 .. 32,767
*    7:   32,768 .. 65,535
*    8:   65,536 .. 131,071
*    9:   131,072 .. 262,143
*    10:  262,144 .. 524,143
*    11:  524,288 .. 1,048,575
*    12:  1,048,576 .. 2,097,151
*    13:  2,097,152 .. 4,192,303
*    14:  4,194,304 .. 8,388,607
*    15:  8,388,608 +
*
* The histogram ranges can be altered using these macros...
*    - INTER_ARRIVAL_HIST_BIN_2_VAL
*    - INTER_ARRIVAL_HIST_SHIFT
*    - INTER_ARRIVAL_HIST_SIZE
*
* As described above, the inter-arrival stats are collected independently for each direction/plane.
*
* Note that TDD patterns, and also RACH opportunity frequency affect the expected histogram distribution.
* Ths is explained below, in a test which has the following srsRAN confif parameters ..
*
* common_scs: 30
*
* prach_config_index: 159       i.e. PRACH occurs every 10 ms frame.
*
* tdd_ul_dl_cfg:
*    dl_ul_tx_period: 10
*    nof_dl_slots: 7
*    nof_dl_symbols: 6
*    nof_ul_slots: 2
*    nof_ul_symbols: 4
*
*
*
* Here is an example of the typical UL inter-arrival stats when zero UE are connected .. 
* "ulPacketStats":{
*     "dataPacketStats":{
*         "PacketCount":4848, 
*         "PrbCount":"58176", 
*         "packetInterArrivalInfo":{
*             "hist":[0, 0, 468, 96, 594, 3367, 218, 4, 0, 0, 0, 0, 0, 0, 0, 101]}}}
* Here it can be seen that the tail bin has close to 100 entries i.e. one for every 10ms RACH opportunity.
*
* Here is an example of the typical UL inter-arrival stats when UE is connected and actively doing uplink traffic connected .. 
* "ulPacketStats":{
*     "dataPacketStats":{
*         "PacketCount":19576, "PrbCount":"1619344", 
*         "packetInterArrivalInfo":{
*              "hist":[0, 0, 1265, 6601, 4295, 4402, 2789, 40, 0, 0, 0, 0, 0, 121, 46, 17]}}}
* In this case it can be seen that the tail bin is smaller due to the fact that the UE is able to transmit more frequently than every 10ms, 
* due to maintaining a connection at the PHY layer.  It can also be seen that the number of packets increases, as expected
*
*
* In the downlink, since the config has setting "nof_ul_slots: 2", it means that there will be a gap of 2 slots in which no DL data will be 
* sent.  Since the config has "common_scs: 30", this means that periodically (i.e. every 0.5ms slot), inter-arrival times will be > 1ms.
* Here is an exmaple of the DL user plane stats ..
* "dlPacketStats":{
*     "dataPacketStats":{

*         "PacketCount":83764, "PrbCount":"8879408",
*         "packetInterArrivalInfo":{
*              "hist":[62244, 403, 128, 46, 4, 2, 8271, 12468, 0, 0, 0, 0, 202, 0, 0, 0]}}, 
* Bin 12 (i.e. 1.048ms .. 2.097ms) has a count of 202. This is as expected to be approx 200 due to the fact that slots 8,9,18,19 
* are UL-only slots, so foL user-plane an inter-arrival time >1ms will be expected every 5ms.
*/

#include "jbpf_defs.h"
#include "jbpf_helper.h"
#include "../utils/misc_utils.h"
#include "xran_packet_info.pb.h"
#include "jbpf_srsran_contexts.h"
#include "xran_format.h"

// map to store data before sending to output_map
struct jbpf_load_map_def SEC("maps") output_tmp_map = {
  .type = JBPF_MAP_TYPE_ARRAY,
  .key_size = sizeof(int),
  .value_size = sizeof(packet_stats),
  .max_entries = 1,
};

// map used to store the timestamps of of when a message was processed for each direction / plane
struct jbpf_load_map_def SEC("maps") last_timestamp = {
    .type = JBPF_MAP_TYPE_ARRAY,
    .key_size = sizeof(int),
    .value_size = sizeof(uint64_t),
    .max_entries = 3,       // 0: uplink u plane, 
                            // 1: downlink u plane, 
                            // 2: downlink c plane
};
#define LAST_TS_UL_UPLANE (0)
#define LAST_TS_DL_UPLANE (1)
#define LAST_TS_DL_CPLANE (2)


#define ECPRI_ETH_TYPE (0xaefe)

#define MAX_PRB (273)

#define INTER_ARRIVAL_HIST_BIN_2_VAL (512)
#define INTER_ARRIVAL_HIST_SHIFT (1)
#define INTER_ARRIVAL_HIST_SIZE (16)



/* 
GET_HIST_BIN - Macro to retrieve bin index

Explanation:
If  __bin2_v = 32:
    The first bin handles 0–31, so the second bin starts at 32.
value < 32:
    Values less than 32 fall into bin 0.
boundary <<= 1:  (e.g. if __shift_size==1)
    The boundary doubles for each subsequent bin (e.g., 64, 128, 256, etc.).
bin < max_bins - 1:
    Ensures the loop does not exceed the maximum number of bins.
Final bin Value:
    The bin variable contains the index of the bin into which the value falls.
*/
#define GET_HIST_BIN(__value, __max_bins, __bin2_v, __shift_size) ({ \
    int __bin = 0; \
    int __boundary = __bin2_v; /* Start with the second bin boundary */ \
    if (__value < __bin2_v) { \
        __bin = 0; /* First bin (0–31) */ \
    } else { \
        while (__value >= __boundary && __bin < __max_bins - 1) { \
            __bin++; \
            __boundary <<= __shift_size; /* Increase the boundary by  (<<__shift_size) */ \
        } \
    } \
    __bin; \
})



SEC("jbpf_ran_ofh")
uint64_t jbpf_main(void *state)
{
    struct jbpf_ran_ofh_ctx *ctx;
    ctx = (struct jbpf_ran_ofh_ctx *)state;
    int zero_index=0;
    int last_timestamp_index=0;
    void *xran_pkt_start;
    void* xran_pkt_end;
    data_packet_stats_item* data_item = NULL;
    packet_inter_arrival_info_item* inter_arrival = NULL;

    // set the packet start and end pointers
    xran_pkt_start = (void *)ctx->data;
    xran_pkt_end = (void *)ctx->data_end;

    // get current timestamp
    //uint64_t timestamp = jbpf_time_get_ns();

    // get access to the temporary output map
    packet_stats *out = (packet_stats *)jbpf_map_lookup_elem(&output_tmp_map, &zero_index);
    if (!out) {
        return -1;    
    }

    // the xran packet is an ethernet frame as defined in https://docs.o-ran-sc.org/projects/o-ran-sc-o-du-phy/en/latest/Transport-Layer-and-ORAN-Fronthaul-Protocol-Implementation_fh.html
    // decode the ethernet type
    struct ethhdr *eh = (struct ethhdr *)xran_pkt_start;
    if ((void*)(eh + 1) >= xran_pkt_end) {
        return -1;
    }

    // point past eth_hdr
    void* next_hdr = (__u8 *)eh + sizeof(struct ethhdr);

    // correct endianness
    //uint16_t ether_type = (eh->h_proto << 8) | (eh->h_proto >> 8);
    uint16_t ether_type = jbpf_xran_ntohs(eh->h_proto);

    // handle if message has VLAN
    if (ether_type == ETH_P_8021Q) {

        struct vlan_hdr *vl_hdr = (struct vlan_hdr *)next_hdr;
        if ((void*)(vl_hdr + 1) >= xran_pkt_end) {
            return -1;
        }

        // correct endianness
        ether_type = jbpf_xran_ntohs(vl_hdr->h_vlan_encapsulated_proto);
        //ether_type = (vl_hdr->h_vlan_encapsulated_proto << 8) | (vl_hdr->h_vlan_encapsulated_proto >> 8);

        // point past the VLAN header
        next_hdr = (__u8 *)next_hdr + sizeof(struct vlan_hdr);
    }

    if (ether_type != ECPRI_ETH_TYPE) {
        return -1;
    }
    
    // move to the ECPRI header
    struct xran_ecpri_hdr *ecpri_hdr = (struct xran_ecpri_hdr *)next_hdr;
    if ((void*)(ecpri_hdr + 1) >= xran_pkt_end) {
        return -1;
    }
    // point past the ECPRI header        
    next_hdr = (__u8 *)next_hdr + sizeof(struct xran_ecpri_hdr);

    if (ecpri_hdr->cmnhdr.bits.ecpri_mesg_type == ECPRI_IQ_DATA)
    {
        // process u plane message

        // move to the radio_app_common_hdr
        struct radio_app_common_hdr *app_common_hdr = (struct radio_app_common_hdr *)next_hdr;
        if ((void*)(app_common_hdr + 1) >= xran_pkt_end) {
            return -1;
        }
        // point past the Radio App common header        
        next_hdr = (__u8 *)next_hdr + sizeof(struct radio_app_common_hdr);

        // move to the data_section_hdr
        struct data_section_hdr *data_sec_hdr = (struct data_section_hdr *)next_hdr;
        if ((void*)(data_sec_hdr + 1) >= xran_pkt_end) {
            return -1;
        }

        if (ctx->direction == XRAN_DIRECTION_UPLINK) { 
            data_item = &out->ul_packet_stats.data_packet_stats;
            last_timestamp_index = LAST_TS_UL_UPLANE;
        } else if (ctx->direction == XRAN_DIRECTION_DOWNLINK) {
            data_item = &out->dl_packet_stats.data_packet_stats;
            last_timestamp_index = LAST_TS_DL_UPLANE;
        } else {
            return -1;
        }

        // increment the packet count
         __sync_fetch_and_add(&data_item->Packet_count, 1);
       
        // add to the PRB count
        uint64_t num_prb = (int64_t)((data_sec_hdr->fields.all_bits >> 24) & 0xff);
        // num_pof of 0 means max PRB (273)
        num_prb = (num_prb == 0) ? MAX_PRB : num_prb;
        __sync_fetch_and_add(&data_item->Prb_count, num_prb);        

        inter_arrival = &data_item->packet_inter_arrival_info;

    }  else if (ecpri_hdr->cmnhdr.bits.ecpri_mesg_type == ECPRI_RT_CONTROL_DATA) {

        // process c plane message

        // we only expect CONTROL in dl
        if (ctx->direction != XRAN_DIRECTION_DOWNLINK) {
            return -1;
        }

        ctrl_packet_stats_item* ctrl_item = &out->dl_packet_stats.ctrl_packet_stats;

        // increment the packet count
        __sync_fetch_and_add(&ctrl_item->Packet_count, 1);

        inter_arrival = &ctrl_item->packet_inter_arrival_info;

        last_timestamp_index = LAST_TS_DL_CPLANE;

    } else {
        return -1;
    }

    // get timestamp of the last message processing for this direction / plane 
    uint64_t *last_ts = jbpf_map_lookup_elem(&last_timestamp, &last_timestamp_index);
    if (!last_ts) {
        return -1;
    }

    uint64_t timestamp = jbpf_time_get_ns();

    uint64_t last_ts_prev = *last_ts;
    
    // if *last_ts_prev==0, this means it is the first message received.  Therefore skip this as we 
    // cannot calculate an inter-arrival time
    if (last_ts_prev == 0) {
        *last_ts = timestamp;
        return 0;
    }

    // update the inter-arrival time stats
    uint64_t inter_arrival_time = timestamp - last_ts_prev;

    *last_ts = timestamp;

    // increment the relevant histogram bin
    uint32_t inter_arrival_bin = GET_HIST_BIN(inter_arrival_time, INTER_ARRIVAL_HIST_SIZE, INTER_ARRIVAL_HIST_BIN_2_VAL, INTER_ARRIVAL_HIST_SHIFT);
    inter_arrival->hist[inter_arrival_bin & (uint32_t)(sizeof(inter_arrival->hist) / sizeof(inter_arrival->hist[0]) - 1)]++;

    return 0;
}
