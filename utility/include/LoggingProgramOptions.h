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


#ifndef BGQ_UTILITY_LOGGING_PROGRAM_OPTIONS_H_
#define BGQ_UTILITY_LOGGING_PROGRAM_OPTIONS_H_


/*!
 * \file utility/include/LoggingProgramOptions.h
 * \brief Program options for logging configuration.
 */

#include <log4cxx/logger.h>

#include <boost/program_options.hpp>

#include <map>
#include <string>
#include <vector>


namespace bgq {
namespace utility {


/*! \brief Program options for logging configuration.
 *

LoggingProgramOptions provides program options for logging configuration.
By adding the options to your program's options you can allow users of your program
to easily change the logging configuration for the program at run-time.
This can make debugging problems easier.

There's only one option provided by LoggingProgramOptions, --verbose.
In short, users can override the logging level for a logger by using an argument like
--verbose ibm.realtime.server=DEBUG .
You can specify --verbose multiple times to set the levels for different loggers.

\htmlinclude verbose.html

If the level is not valid then std::invalid_argument will be thrown when boost::program_options::notify() is called.

To use this class
-# Create an instance, providing the default logger name: <pre>bgq::utility::LoggingProgramOptions logging_program_options( "ibm.realtime" );</pre>
-# Call addTo() with your options_description
-# When boost::program_options::notify() is called, the arguments are set in the instance.
-# Pass the instance to initializeLogging().


Code example: (for a full example, see bgq/hlcs/src/realtime/bg_realtime_server.cc. The entire file is only 100 lines.)

<pre>

int main( int argc, char* argv[] )
{
    namespace po = boost::program_options;

    po::options_description options( "Options" );

    // Add other options.

    bgq::utility::LoggingProgramOptions logging_program_options( "ibm.realtime" );

    logging_program_options.addTo( options );

    po::variables_map vm;

    po::store(po::parse_command_line( argc, argv, options ), vm); // don't forget to catch exceptions.

    po::notify( vm );

    bgq::utility::initializeLogging(
            *properties_ptr,
            logging_program_options
        );

    ...
}

</pre>

 *
 */
class LoggingProgramOptions
{
public:

    typedef std::vector<std::string> Strings; //!< Argument for notifier.

    typedef std::map<std::string,log4cxx::LevelPtr> LoggingLevels; //!< loggers to set


    /*! \brief Constructor.
     */
    explicit LoggingProgramOptions(
            const std::string& default_logger_name
        );

    /*! \brief Add the program options to the options description.
     *
     */
    void addTo(
            boost::program_options::options_description& opts_desc
        );

    /*! \brief Set the parameters.
     *
     * This is typically called automatically when boost::program_options::notify is called.
     *
     * \param args parameters to --verbose,
     * as described in the detailed description of LoggingProgramOptions.
     *
     * \throws std::invalid_argument if the format of the parameter is not valid.
     */
    void notifier(
            const Strings& args
        );

    /*! \brief Apply the settings to the logging configuration.
     *
     * This is called by initializeLogging() when the LoggingProgramOptions is passed to it.
     *
     */
    void apply() const;


    /*! \brief Parse a verbose argument.
     *
     * \throws std::invalid_argument if the format of the parameter is not valid.
     */
    static log4cxx::LevelPtr parseVerboseArgument(
            const std::string& str  //!< [in]
            );

    /*! \brief Get the logging levels set after parsing.
     */
    const LoggingLevels& get() const { return _logging_levels; }

private:

    std::string _default_logger_name;
    LoggingLevels _logging_levels;


    void _parseVerboseString(
            const std::string& str,
            std::string& logger_name_out,
            log4cxx::LevelPtr& level_ptr_out
        );
};


} // namespace bgq::utility
} // namespace bgq


#endif
