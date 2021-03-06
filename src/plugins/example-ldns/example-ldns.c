
#include <stdio.h>
#include <string.h>

#include <dnscrypt/plugin.h>
#include <ldns/ldns.h>

int
dcplugin_init(DCPlugin * const dcplugin, int argc, char *argv[])
{
    return 0;
}

int
dcplugin_destroy(DCPlugin * const dcplugin)
{
    return 0;
}

DCPluginSyncFilterResult
dcplugin_sync_pre_filter(DCPlugin *dcplugin, DCPluginDNSPacket *dcp_packet)
{
    uint8_t  *new_packet;
    ldns_rdf *bing, *google, *owner;
    ldns_pkt *packet;
    ldns_rr  *question;
    size_t    new_packet_size;

    ldns_wire2pkt(&packet, dcplugin_get_wire_data(dcp_packet),
                  dcplugin_get_wire_data_len(dcp_packet));

    question = ldns_rr_list_rr(ldns_pkt_question(packet), 0U);
    owner = ldns_rr_owner(question);

    bing = ldns_dname_new_frm_str("*.bing.com.");
    if (ldns_dname_match_wildcard(owner, bing)) {
        google = ldns_dname_new_frm_str("www.google.com.");
        ldns_rdf_free(owner);
        ldns_rr_set_owner(question, google);
        ldns_pkt2wire(&new_packet, packet, &new_packet_size);
        if (dcplugin_get_wire_data_max_len(dcp_packet) >= new_packet_size) {
            dcplugin_set_wire_data(dcp_packet, new_packet, new_packet_size);
        }
        free(new_packet);
    }

    ldns_pkt_free(packet);
    ldns_rdf_free(bing);

    return DCP_SYNC_FILTER_RESULT_OK;
}

DCPluginSyncFilterResult
dcplugin_sync_post_filter(DCPlugin *dcplugin, DCPluginDNSPacket *dcp_packet)
{
    uint8_t      *new_packet;
    ldns_rr_list *answers;
    ldns_rr      *answer;
    ldns_pkt     *packet;
    size_t        answers_count, i, new_packet_size;
    _Bool         should_be_blocked = 0;

    ldns_wire2pkt(&packet, dcplugin_get_wire_data(dcp_packet),
                  dcplugin_get_wire_data_len(dcp_packet));

    answers = ldns_pkt_answer(packet);
    answers_count = ldns_rr_list_rr_count(answers);
    for (i = 0U; i < answers_count; i++) {
        answer = ldns_rr_list_rr(answers, i);
        if (ldns_rr_get_type(answer) == LDNS_RR_TYPE_A) {
            if (strcmp("157.166.255.18",
                       ldns_rdf2str(ldns_rr_a_address(answer))) == 0) {
                should_be_blocked = 1;
                break;
            }
        }
    }
    if (should_be_blocked != 0) {
        ldns_pkt_set_rcode(packet, LDNS_RCODE_NXDOMAIN);
        ldns_pkt2wire(&new_packet, packet, &new_packet_size);
        dcplugin_set_wire_data(dcp_packet, new_packet, new_packet_size);
        free(new_packet);
    }

    ldns_pkt_free(packet);

    return DCP_SYNC_FILTER_RESULT_OK;
}
