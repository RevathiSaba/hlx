//: ----------------------------------------------------------------------------
//: subrequest example:
//: compile with:
//:   g++ subrequest.cc -lhlxcore -lssl -lcrypto -lpthread -o subrequest
//: ----------------------------------------------------------------------------
#include <hlx/srvr.h>
#include <hlx/lsnr.h>
#include <hlx/proxy_h.h>
#include <hlx/default_rqst_h.h>
#include <hlx/url_router.h>
#include <hlx/api_resp.h>
#include <hlx/resp.h>
#include <hlx/rqst.h>
#include <hlx/subr.h>
#include <hlx/trace.h>

#include <string.h>
#include <unistd.h>
//#include <google/profiler.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif
#include <inttypes.h>

ns_hlx::srvr *g_srvr = NULL;

class default_h: public ns_hlx::default_rqst_h
{
public:
        // GET
        ns_hlx::h_resp_t do_default(ns_hlx::clnt_session &a_clnt_session,
                                    ns_hlx::rqst &a_rqst,
                                    const ns_hlx::url_pmap_t &a_url_pmap)
        {
                char l_len_str[64];
                uint32_t l_body_len = strlen("Hello World\n");
                sprintf(l_len_str, "%u", l_body_len);
                ns_hlx::api_resp &l_api_resp = create_api_resp(a_clnt_session);
                l_api_resp.set_status(ns_hlx::HTTP_STATUS_OK);
                l_api_resp.set_header("Content-Length", l_len_str);
                l_api_resp.set_body_data("Hello World\n", l_body_len);
                ns_hlx::queue_api_resp(a_clnt_session, l_api_resp);
                return ns_hlx::H_RESP_DONE;
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
        // Suppress errors
        ns_hlx::trc_log_level_set(ns_hlx::TRC_LOG_LEVEL_NONE);
        ns_hlx::lsnr *l_lsnr = new ns_hlx::lsnr(12345, ns_hlx::SCHEME_TCP);
        ns_hlx::proxy_h *l_proxy_h = new ns_hlx::proxy_h("http://127.0.0.1:12346", "/proxy");
        l_proxy_h->set_timeout_ms(1000);
        l_proxy_h->set_max_parallel(2);
        ns_hlx::rqst_h *l_default_h = new default_h();
        ns_hlx::rqst_h *l_rqst_h_quit = new quitter();
        l_lsnr->add_route("/proxy/*", l_proxy_h);
        l_lsnr->add_route("/quit", l_rqst_h_quit);
        l_lsnr->set_default_route(l_default_h);
        g_srvr = new ns_hlx::srvr();
        g_srvr->register_lsnr(l_lsnr);
        g_srvr->set_num_threads(0);
        //g_srvr->set_num_threads(1);
        //g_srvr->set_rqst_resp_logging(true);
        //g_srvr->set_rqst_resp_logging_color(true);
        //ProfilerStart("tmp.prof");
        g_srvr->run();
        //sleep(1);
        //while(g_srvr->is_running())
        //{
        //        sleep(1);
        //        //g_srvr->display_stats();
        //}
        //ProfilerStop();
        if(g_srvr) {delete g_srvr; g_srvr = NULL;}
        if(l_proxy_h) {delete l_proxy_h; l_proxy_h = NULL;}
        if(l_rqst_h_quit) {delete l_rqst_h_quit; l_rqst_h_quit = NULL;}
}
