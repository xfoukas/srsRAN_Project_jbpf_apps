#include "jbpf_defs.h"
#include "jbpf_helper.h"
#include "xran_packet_info.pb.h"
#include "jbpf_srsran_contexts.h"
#include "xran_format.h"

// output map for stats
jbpf_ringbuf_map(output_map, packet_stats, 256);

// map to store data before sending to output_map
struct jbpf_load_map_def SEC("maps") output_tmp_map = {
  .type = JBPF_MAP_TYPE_ARRAY,
  .key_size = sizeof(int),
  .value_size = sizeof(packet_stats),
  .max_entries = 1,
};



SEC("jbpf_stats")
uint64_t
jbpf_main(void* state)
{
    int zero_index=0;
    packet_stats *out = (packet_stats *)jbpf_map_lookup_elem(&output_tmp_map, &zero_index);
    if (!out) {
        return -1;    
    }

    uint64_t timestamp = jbpf_time_get_ns();

    out->timestamp = timestamp;
        
    jbpf_ringbuf_output(&output_map, (void *)out, sizeof(packet_stats));

    // zero the stats
    jbpf_map_clear(&output_tmp_map);

    //uint64_t u64_fetch_and_res = __sync_fetch_and_and(&out->ul_packet_stats.data_packet_stats.Packet_count, 0x0);

    return 0;
}

