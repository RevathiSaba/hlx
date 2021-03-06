//: ----------------------------------------------------------------------------
//: fanout example:
//: compile with:
//:   g++ floodecho.cc -lhlxcore -lssl -lcrypto -lpthread -o floodecho
//: ----------------------------------------------------------------------------
#include <hlx/srvr.h>
#include <hlx/phurl_h.h>
#include <hlx/stat_h.h>
#include <hlx/lsnr.h>
#include <hlx/api_resp.h>
#include <hlx/resp.h>

#include <string.h>
#include <stdlib.h>
//#include <google/profiler.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif
#include <inttypes.h>

ns_hlx::srvr *g_srvr = NULL;

class hello_from: public ns_hlx::phurl_h
{
public:
        int32_t create_resp(ns_hlx::subr &a_subr, ns_hlx::phurl_u *a_phurl_u)
        {
                // Get body of resp
                char l_buf[4096];
                l_buf[0] = '\0';
                for(ns_hlx::hlx_resp_list_t::iterator i_resp = a_phurl_u->m_resp_list.begin();
                    i_resp != a_phurl_u->m_resp_list.end();
                    ++i_resp)
                {
                        char *l_status_buf = NULL;
                        int l_as_len = asprintf(&l_status_buf, "Hello From: %s -STATUS: %u\n",
                                        (*i_resp)->m_subr->get_host().c_str(),
                                        (*i_resp)->m_resp->get_status());
                        strncat(l_buf, l_status_buf, l_as_len);
                        free(l_status_buf);
                }
                uint64_t l_len = strnlen(l_buf, 2048);

                // Create length string
                char l_len_str[64];
                sprintf(l_len_str, "%" PRIu64 "", l_len);

                // Create resp
                ns_hlx::api_resp &l_api_resp = ns_hlx::create_api_resp(*(a_subr.get_clnt_session()));
                l_api_resp.set_status(ns_hlx::HTTP_STATUS_OK);
                l_api_resp.set_header("Content-Length", l_len_str);
                l_api_resp.set_body_data(l_buf, l_len);

                // Queue
                ns_hlx::queue_api_resp(*(a_subr.get_clnt_session()), l_api_resp);
                delete a_phurl_u;
                return 0;
        }
};

class quitter: public ns_hlx::default_rqst_h
{
public:
        // GET
        ns_hlx::h_resp_t do_get(ns_hlx::clnt_session &a_clnt_session,
                                ns_hlx::rqst &a_rqst,
                                const ns_hlx::url_pmap_t &a_url_pmap)
        {
                if(g_srvr)
                {
                        g_srvr->stop();
                }
                return ns_hlx::H_RESP_DONE;
        }
};

int main(void)
{
        ns_hlx::lsnr *l_lsnr = new ns_hlx::lsnr(12345, ns_hlx::SCHEME_TCP);
        hello_from *l_hello_from = new hello_from();
        ns_hlx::stat_h *l_stat_h = new ns_hlx::stat_h();

        // ---------------------------------------
        // Setup parallel hurl endpoint
        // ---------------------------------------
        // Add hosts
        l_hello_from->add_host("www.yahoo.com", 443);
        l_hello_from->add_host("www.google.com", 443);
        l_hello_from->add_host("www.reddit.com", 443);

        // Setup subr template
        ns_hlx::subr &l_subr = l_hello_from->get_subr_template_mutable();
        l_subr.set_port(443);
        l_subr.set_scheme(ns_hlx::SCHEME_TLS);

        // Add endpoints
        l_lsnr->add_route("/phurl", l_hello_from);
        l_lsnr->add_route("/stat", l_stat_h);
        l_lsnr->add_route("/quit", new quitter());

        g_srvr = new ns_hlx::srvr();
        g_srvr->register_lsnr(l_lsnr);
        g_srvr->set_num_threads(0);
        g_srvr->set_num_parallel(32);
        g_srvr->set_update_stats_ms(500);

        //g_srvr->set_rqst_resp_logging(true);
        //g_srvr->set_rqst_resp_logging_color(true);

        //ProfilerStart("tmp.prof");
        g_srvr->run();
        delete g_srvr;
        g_srvr = NULL;
        //ProfilerStop();
}
