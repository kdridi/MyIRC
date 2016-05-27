#define BOOST_TEST_MODULE MyIRC
#include <boost/test/included/unit_test.hpp>

#include <libircclient.h>
#include <strings.h>
#include <stdlib.h>

typedef enum {
    kStatusDisconnected,
    kStatusConnected,
} Status;

struct Context {
    Context()
    : server(boost::unit_test::framework::master_test_suite().argv[1])
    , port(atoi(boost::unit_test::framework::master_test_suite().argv[2]))
    , nick(boost::unit_test::framework::master_test_suite().argv[3])
    , username(boost::unit_test::framework::master_test_suite().argv[4])
    , realname(boost::unit_test::framework::master_test_suite().argv[5])
    , status(kStatusDisconnected)
    {
        BOOST_CHECK_MESSAGE(boost::unit_test::framework::master_test_suite().argc == 6, "Usage: test [host] [port] [nick] [username] [realname]");
    }
    
    std::string const server;
    unsigned short const port;
    std::string const nick;
    std::string const username;
    std::string const realname;
    Status status;
};

BOOST_FIXTURE_TEST_CASE(simple_connection, Context)
{
    int status;
    
    irc_callbacks_t callbacks;
    bzero(&callbacks, sizeof(irc_callbacks_t));
    callbacks.event_connect = [] (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
    {
        Context &ctx{*(static_cast<Context *>(irc_get_ctx(session)))};
        ctx.status = kStatusConnected;
        
        BOOST_CHECK_MESSAGE(count > 1, "Must have at leat one param" );
        BOOST_CHECK_MESSAGE(ctx.nick.compare(params[0]) == 0, "The first param should be the nick" );

        irc_cmd_quit(session, "");
    };
    
    irc_session_t *session = irc_create_session(&callbacks);
    BOOST_CHECK_MESSAGE(session, "Can't create a new session");
    
    irc_option_set(session, LIBIRC_OPTION_STRIPNICKS);
    
    status = irc_connect(session, server.c_str(), port, NULL, nick.c_str(), username.c_str(), realname.c_str());
    BOOST_CHECK_MESSAGE(status == 0, "Unable to connect to the server");
    
    irc_set_ctx(session, this);
    
    while ( irc_is_connected(session) )
    {
        struct timeval tv;
        fd_set in_set, out_set;
        int maxfd = 0;
        
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        // Init sets
        FD_ZERO (&in_set);
        FD_ZERO (&out_set);
        
        irc_add_select_descriptors (session, &in_set, &out_set, &maxfd);
        
        status = select (maxfd + 1, &in_set, &out_set, 0, &tv);
        BOOST_CHECK_MESSAGE(status > 0, "Timeout ERROR");
        
        if (irc_process_select_descriptors (session, &in_set, &out_set)) {
            int const error = irc_errno(session);
            BOOST_CHECK_MESSAGE(error == LIBIRC_ERR_TERMINATED, "Reason must be LIBIRC_ERR_TERMINATED");
            BOOST_CHECK_MESSAGE(this->status == kStatusConnected, "Status must be connected");
            BOOST_CHECK_MESSAGE(irc_is_connected(session) == 0, "Status must be connected");
        }
    }
    
    irc_destroy_session(session);
}
