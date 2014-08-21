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

#include "Log.h"

#include "LoggingProgramOptions.h"
#include "Properties.h"

#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/patternlayout.h>
#include <log4cxx/propertyconfigurator.h>

#include <log4cxx/helpers/properties.h>

#include <sstream>


using std::ostringstream;
using std::string;


LOG_DECLARE_FILE( "utility.logging" );


namespace bgq {
namespace utility {


const std::string DEFAULT_LOGGING_SUBSECTION( "default" );


const string LoggingPropertiesFilenameEnvVarName( "BG_LOGGING_PROPERTIES_FILE" );
const string BgPropertiesLoggingSectionName( "logging" );
const string DefaultConversionPattern( "%d{yyyy-MM-dd HH:mm:ss.SSS} (%-5p) [%t] %c: %m%n" );


static bool logging_initialized(false);


static string stripExtension( const string& file )
{
    // find where the filename starts.
    // - if file has a '/' then it's the char after the last '/'.
    // - otherwise it's the start of file.

    string::size_type start_pos(file.rfind( '/' ));
    if ( start_pos == string::npos ) {
        // didn't find a '/'.
        start_pos = 0;
    } else {
        ++start_pos;
    }

    // strip the extension.. it's everything after the first '.' from where the filename starts.

    string::size_type end_pos(file.find( '.', start_pos ));
    if ( start_pos == end_pos ) {
        end_pos = file.find( '.', end_pos + 1 );
    }

    string ret(file.substr( 0, end_pos ));

    return ret;
}


static void replaceChars( string& file_in_out )
{
    // replace any '/' with '.' and '.' with '_'
    for ( string::iterator i(file_in_out.begin()) ; i != file_in_out.end() ; ++i ) {
        if ( *i == '/' ) {
            *i = '.';
        } else if ( *i == '.' ) {
            *i = '_';
        }
    }
}


static string processFilename( const string& file )
{
    string ret(stripExtension( file ));
    replaceChars( ret );

    return ret;
}


static void mergeProperties(
        log4cxx::helpers::Properties& logging_properties_in_out,
        const Properties& bg_properties,
        const std::string& section_name
    )
{
    const Properties::Section &section(bg_properties.getValues( section_name )); // Throws if the section doesn't exist.

    // Loop through the properties in the section, adding them to the log4cxx properties.
    for ( Properties::Section::const_iterator i(section.begin()) ; i != section.end() ; ++i ) {
        logging_properties_in_out.setProperty( i->first, i->second );
    }
}


static void useDefaultConfiguration()
{
    log4cxx::LayoutPtr layout_ptr( new log4cxx::PatternLayout( DefaultConversionPattern ) );

    log4cxx::AppenderPtr appender_ptr( new log4cxx::ConsoleAppender( layout_ptr ) );

    log4cxx::BasicConfigurator::configure( appender_ptr );
}



// External definitions


//---------------------------------------------------------------------
// functions


void initializeLogging(
        const Properties& bg_properties,
        const std::string& subsection_name
    )
{
    if ( logging_initialized ) {
        LOG_DEBUG_MSG( "Logging already initialized." );
        return;
    }

    logging_initialized = true;

    // if BG_LOGGING_PROPERTIES_FILE is set then read logging properties from that file
    // rather than from the properties.

    const char *filename_env(getenv( LoggingPropertiesFilenameEnvVarName.c_str() ));

    if ( filename_env ) {
        string logging_properties_filename(filename_env);
        log4cxx::PropertyConfigurator::configure( log4cxx::File( logging_properties_filename ) );

        LOG_INFO_MSG( "Read logging configuration from '" << logging_properties_filename << "'." );
        return;
    }

    // The env var wasn't set, so get the logging configuration from bg_properties.

    // Convert the properties in the logging and logging.<subsection> sections of the
    // bg.properties file to log4cxx properties and then use that to configure log4cxx.

    try {
        log4cxx::helpers::Properties properties;

        mergeProperties( properties, bg_properties, BgPropertiesLoggingSectionName );

        bool invalid_subsection(false); // will be set to true if couldn't find subsection

        const string subsection_full_name(BgPropertiesLoggingSectionName + "." + subsection_name);

        try {

            mergeProperties( properties, bg_properties, subsection_full_name );

        } catch ( const std::invalid_argument& inv_arg ) {

            invalid_subsection = true;

            // The subsection typically contains the default appender in log4j.appender.default.
            // If that property isn't present in the properties then PropertyConfigurator::configure() will
            // print out an ugly message. So add default values for log4j.appender.default.

            if ( properties.get( "log4j.appender.default" ).empty() ) {
                properties.setProperty( "log4j.appender.default", "org.apache.log4j.ConsoleAppender" );
                properties.setProperty( "log4j.appender.default.layout", "org.apache.log4j.PatternLayout" );
                properties.setProperty( "log4j.appender.default.layout.ConversionPattern", DefaultConversionPattern );
            }
        }

        log4cxx::PropertyConfigurator::configure( properties );

        LOG_DEBUG_MSG( "Logging configured." );

        if ( invalid_subsection ) {
            LOG_WARN_MSG(
                    "Error reading logging subsection '" << subsection_full_name << "' in properties file '" << bg_properties.getFilename() << "'."
                    " Check the properties file."
                );
        }
    } catch ( const std::invalid_argument& inv_arg ) {
        // In this case the logging section couldn't be read. Use a default configuration.

        useDefaultConfiguration();

        LOG_WARN_MSG(
                "Could not read logging section '" << BgPropertiesLoggingSectionName << "' in properties file '" << bg_properties.getFilename() << "'."
                " Check the properties file."
                " Will use the default configuration."
            );
    }
}


void initializeLogging(
        const Properties& bg_properties,
        const LoggingProgramOptions& logging_program_options,
        const std::string& subsection_name
    )
{
    initializeLogging( bg_properties, subsection_name );

    logging_program_options.apply();
}


string calcLoggername( const string& base, const string& file )
{
    ostringstream oss;
    oss << "ibm." << base << "." << processFilename( file );
    return oss.str();
}


} // namespace bgq::util
} // namespace bgq
