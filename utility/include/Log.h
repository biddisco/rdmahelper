/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/* ================================================================ */
/*                                                                  */
/* Licensed Materials - Property of IBM                             */
/*                                                                  */
/* Blue Gene/Q                                                      */
/*                                                                  */
/* (C) Copyright IBM Corp.  2010, 2011                              */
/*                                                                  */
/* US Government Users Restricted Rights -                          */
/* Use, duplication or disclosure restricted                        */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/* This software is available to you under the                      */
/* Eclipse Public License (EPL).                                    */
/*                                                                  */
/* ================================================================ */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*!
 * \file utility/include/Log.h
 *
 * \brief Common logging functions.
 *

<h3>Using Log</h3>

A simple application just wants to write log messages and not worry about configuration and all the extra features of the log4cxx library.
On Blue Gene/Q this is provided by Log.
Log does these things
- Reads logging configuration during initialization
- Creates a statically allocated logger for each file that does logging
- Provides macros to log to the static logger at the different levels

To use Log, the application must at some point initialize logging, this should be done early in main(), and of course before any logging is done.

<pre>
\#include <utility/include/Log.h>

string bg_properties_filename;
// ... set bg_properties_filename from a command line option
bgq::utility::Properties properties( bg_properties_filename );
bgq::utility::initializeLogging( properties );
</pre>

\sa bgq::utility::LoggingProgramOptions

Your .cc files that do logging should contain:
<pre>
\#include <utility/include/Log.h>

LOG_DECLARE_FILE( &quot;<i>component</i>&quot; );
</pre>

where component is a name you choose for your component. Each of the files in your component should use the same component name. You can specify subcomponents by separating with '.'. For example, we use "bgsched" as a component and "bgsched.allocator".

To print out a log message, use one of the macros from Log.h:
<pre>
LOG_DEBUG_MSG( expr )
LOG_TRACE_MSG
LOG_INFO_MSG
LOG_WARN_MSG
LOG_ERROR_MSG
LOG_FATAL_MSG
</pre>

The following macros always print the message, ignoring the level:
<pre>
LOG_DEBUG_MSG_FORCED( expr )
LOG_TRACE_MSG_FORCED
LOG_INFO_MSG_FORCED
LOG_WARN_MSG_FORCED
LOG_ERROR_MSG_FORCED
LOG_FATAL_MSG_FORCED
</pre>

You can put an expression as the parameter to the log macros, like
<pre>
LOG_INFO_MSG( "Opening file '" << filename << "'" );
</pre>

LOG_DECLARE_FILE declares a static Logger named &quot;ibm.<i>component</i>.<i>filename-base</i>&quot;.
For example, hlcs/src/bgsched/HardwareConfig.cc has
LOG_DECLARE_FILE( "bgsched" ),
so a static Logger named "ibm.bgsched.HardwareConfig" is created.
Any LOGs in that file will go to the ibm.bgsched.HardwareConfig logger.

The logging level for each logger can be set independently.
This is done in the logging configuration.
The logging configuration is typically in the [logging] section of the bgsched.properties file,
but can be overridden using the BG_LOGGING_PROPERTIES_FILE environment variable.

If you use LOG_DECLARE_FILE( "component" ),
the logger for the component should be added to the utility/etc/bg.properties.template file,
like this:

<pre>
log4j.logger.ibm.<i>component</i> = INFO  # description
</pre>

It may be useful during development to read the logging configuration from a file other than bg.properties.
To use a different file, set the BG_LOGGING_PROPERTIES_FILE environment variable to bypass the logging configuration
in the bg_properties file and read the logging configuration from the BG_LOGGING_PROPERTIES_FILE file.
The format of the BG_LOGGING_PROPERTIES_FILE is as described in the log4cxx documentation for PropertyConfigurator(File).

To get extra logging messages for your component,
edit the logger configuration for the component. For example:
<pre>
log4j.logger.ibm.bgsched = TRACE
</pre>

Because Log creates a separate logger per file, you can change the level for a file.
For example, you could add a line like this to your logging configuration to get trace messages from the HardwareConfig file:
<pre>
log4j.logger.ibm.bgsched.HardwareConfig = TRACE
</pre>

<h3>Using mapped diagnostic context (MDC)</h3>

A mapped diagnostic context is useful for distinguishing interleaved log output from different sources.
One common use of this is a server that can process requests from several clients simultaneously,
making it useful to see each client's ID in the log message.
This is done by putting the client ID in the MDC under some key (e.g., clientId)
and then having a log format that displays the value of the key.

The preferred way to put a value in the MDC is to create a log4cxx::MDC object.
When this object goes out of scope the value at the key is cleared.
Following is an example of setting the a client's ID in the MDC:

<pre>
\#include <log4cxx/mdc.h>

\#define LOGGING_DECLARE_CLIENT_ID_MDC \
 log4cxx::MDC mdc_( "clientId", std::string("clientId=") + _id + " " );

class Client {
private:
  string _id;

public:
  void processClient()
  {
    LOGGING_DECLARE_CLIENT_ID_MDC;

     ... process the client.
  }

};
</pre>

To display the clientId in the log, the layoutPattern contains "%X{clientId}".
To have your application use a different layout pattern than the default,
create a subsection in bg.properties like logging.<i>applicationId</i>.
For example, mc_server might use:

<pre>
[logging.mc_server]

log4j.appender.default = org.apache.log4j.ConsoleAppender
log4j.appender.default.layout = org.apache.log4j.PatternLayout
log4j.appender.default.layout.ConversionPattern = \%d{MMM dd HH:mm:ss.SSS} (\%-5p) [\%t] \%c: \%X{clientId}\%m\%n
</pre>

Then when initializing logging, pass the subsection name into bgq::utility::initializeLogging(), like this:

<pre>
const std::string logging_subsection_name( "mc_server" );
bgq::utility::initializeLogging( properties, logging_subsection_name );
</pre>


<h3>Build instructions</h3>

When compiling, you will need to set
<pre>
CXXFLAGS += -isystem \$(BGQ_INSTALL_DIR)
CXXFLAGS += -isystem \$(BGQ_INSTALL_DIR)/extlib/include
</pre>

For linking, you probably want to do something like:

<pre>
utility_library_dir=\$(BGQ_INSTALL_DIR)/utility/lib
</pre>

Then use these LDFLAGs:
<pre>
LDFLAGS += -L\$(utility_library_dir) -lbgutility -Wl,-rpath,\$(utility_library_dir)
</pre>

 *
 */

#ifndef BGQ_UTILITY_LOG_H_
#define BGQ_UTILITY_LOG_H_

//#include <extlib/include/log4cxx/logger.h>
#include <log4cxx/logger.h>

#include <string>

#ifdef NDEBUG
 // automatically disable logging in release mode
 #define DISABLE_LOGGING
#else
 //#define DISABLE_LOGGING
#endif

namespace bgq {
namespace utility {


class LoggingProgramOptions;
class Properties;


/*! \brief The logging subsection used when no subsection name is given, 'default'.
 */
extern const std::string DEFAULT_LOGGING_SUBSECTION;


/*! \brief Initialize logging from the properties file and program options.
 *
 *  This function configures the logging library based on the contents of the
 *  Properties. It gets the general logging configuration from the "logging".
 *  It also merges configuration from the logging.<i>section</i> section,
 *  which is by default "logging.default".
 *  It also applies the LoggingProgramOptions to the current logging configuration.
 *
 *  Note on threading: The first time this function is called the program must
 *  be single-threaded. In short, call it early in main().
 *
 *  If initializeLogging has already been called this function just returns
 *  without changing any logging.
 *
 *  Set the BG_LOGGING_PROPERTIES_FILE environment variable to bypass the
 *  logging configuration in the bg_properties file and read the logging
 *  configuration from the BG_LOGGING_PROPERTIES_FILE file. The format
 *  of the BG_LOGGING_PROPERTIES_FILE is as described in the log4cxx
 *  documentation for PropertyConfigurator(File).
 *
 */
void initializeLogging(
        const Properties& bg_properties,
        const LoggingProgramOptions& logging_program_options,
        const std::string& subsection_name = DEFAULT_LOGGING_SUBSECTION
    );


/*!
 *  \brief Initialize logging from the properties file.
 *
 */
void initializeLogging(
        const Properties& bg_properties,
        const std::string& subsection_name = DEFAULT_LOGGING_SUBSECTION
    );


/*!
 *  \brief Used internally by LOG_DECLARE_FILE.
 */
std::string calcLoggername( const std::string& base, const std::string& file );


/*!
 * \brief Declare the logger for this file.
 *
 * The resulting logger name will be: "ibm." + base + "." + processed_filename
 * Where the filename is processed to remove the extension (.cc)
 * and any /s are turned into . and .s are turned into _.
 * For example:
 * LOG_DECLARE_FILE( "bgsched" ) in allocator/Allocator.cc becomes
 * "ibm.bgsched.allocator.Allocator"
 */
#define LOG_DECLARE_FILE( base ) \
  static log4cxx::LoggerPtr log_logger_(\
      log4cxx::Logger::getLogger( "ibm" )\
    )

//#define LOG_DECLARE_FILE( base ) \
//  static log4cxx::LoggerPtr log_logger_(\
//      log4cxx::Logger::getLogger( bgq::utility::calcLoggername( (base), __FILE__ ) )\
//    )


// The following macros write log messages at different log levels.
// Note that the message_expr parameter can be an expression, such as:
// "the error from function is " << return_code
#ifdef RDMAHELPER_DISABLE_LOGGING
  #define LOG_DEBUG_MSG(x)
  #define LOG_TRACE_MSG(x)
  #define LOG_INFO_MSG(x)
  #define LOG_WARN_MSG(x)
#define LOG_ERROR_MSG( message_expr ) \
 LOG4CXX_ERROR( log_logger_, message_expr )

#define LOG_FATAL_MSG( message_expr ) \
 LOG4CXX_FATAL( log_logger_, message_expr )

#else


#define LOG_DEBUG_MSG( message_expr ) \
 LOG4CXX_DEBUG( log_logger_, message_expr )

#define LOG_TRACE_MSG( message_expr ) \
 LOG4CXX_DEBUG( log_logger_, message_expr )

#define LOG_INFO_MSG( message_expr ) \
 LOG4CXX_INFO( log_logger_, message_expr )

#define LOG_WARN_MSG( message_expr ) \
 LOG4CXX_WARN( log_logger_, message_expr )

#define LOG_ERROR_MSG( message_expr ) \
 LOG4CXX_ERROR( log_logger_, message_expr )

#define LOG_FATAL_MSG( message_expr ) \
 LOG4CXX_FATAL( log_logger_, message_expr )


#define LOG_DEBUG_MSG_FORCED( message_expr ) { \
    ::log4cxx::helpers::MessageBuffer oss_; \
    log_logger_->forcedLog(::log4cxx::Level::getDebug(), oss_.str(oss_ << message_expr), LOG4CXX_LOCATION); }

#define LOG_TRACE_MSG_FORCED( message_expr ) { \
    ::log4cxx::helpers::MessageBuffer oss_; \
    log_logger_->forcedLog(::log4cxx::Level::getTrace(), oss_.str(oss_ << message_expr), LOG4CXX_LOCATION); }

#define LOG_INFO_MSG_FORCED( message_expr ) { \
    ::log4cxx::helpers::MessageBuffer oss_; \
    log_logger_->forcedLog(::log4cxx::Level::getInfo(), oss_.str(oss_ << message_expr), LOG4CXX_LOCATION); }

#define LOG_WARN_MSG_FORCED( message_expr ) { \
    ::log4cxx::helpers::MessageBuffer oss_; \
    log_logger_->forcedLog(::log4cxx::Level::getWarn(), oss_.str(oss_ << message_expr), LOG4CXX_LOCATION); }

#define LOG_ERROR_MSG_FORCED( message_expr ) { \
    ::log4cxx::helpers::MessageBuffer oss_; \
    log_logger_->forcedLog(::log4cxx::Level::getError(), oss_.str(oss_ << message_expr), LOG4CXX_LOCATION); }

#define LOG_FATAL_MSG_FORCED( message_expr ) { \
    ::log4cxx::helpers::MessageBuffer oss_; \
    log_logger_->forcedLog(::log4cxx::Level::getFatal(), oss_.str(oss_ << message_expr), LOG4CXX_LOCATION); }

#endif // disable logging

} // namespace bgq::utility
} // namespace bgq


#endif

