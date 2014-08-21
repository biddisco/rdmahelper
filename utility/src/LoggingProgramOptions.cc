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


#include <LoggingProgramOptions.h>

#include <Log.h>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>
#include <stdexcept>


using boost::bind;
using boost::shared_ptr;

using log4cxx::Level;
using log4cxx::LevelPtr;
using log4cxx::Logger;

using std::string;


LOG_DECLARE_FILE( "utility" );


static bool debug(false);


namespace bgq {
namespace utility {


LevelPtr
LoggingProgramOptions::parseVerboseArgument( 
        const string& str
        )
{
    // Single letters.
    if ( str == "O" || str == "o" )  return Level::getOff();
    if ( str == "F" || str == "f" )  return Level::getFatal();
    if ( str == "E" || str == "e" )  return Level::getError();
    if ( str == "W" || str == "w" )  return Level::getWarn();
    if ( str == "I" || str == "i" )  return Level::getInfo();
    if ( str == "D" || str == "d" )  return Level::getDebug();
    if ( str == "T" || str == "t" )  return Level::getTrace();
    if ( str == "A" || str == "a" )  return Level::getAll();

    try {
        int level_int(boost::lexical_cast<int> ( str ));

        switch ( level_int ) {
            case 0: return Level::getOff();
            case 1: return Level::getFatal();
            case 2: return Level::getError();
            case 3: return Level::getWarn();
            case 4: return Level::getInfo();
            case 5: return Level::getDebug();
            case 6: return Level::getTrace();
            case 7: return Level::getAll();
        }

        BOOST_THROW_EXCEPTION( std::invalid_argument( string() + "invalid verbose option '" + str + "'" ) );
    } catch ( boost::bad_lexical_cast& e ) {
        // Ignore this exception and try another method.
    }

    LevelPtr ret(Level::toLevel( str ));

    // To figure out if the level string wasn't recognized,
    if ( ret != Level::toLevel( str, Level::getOff() ) ) {
        BOOST_THROW_EXCEPTION( std::invalid_argument( string() + "invalid verbose option '" + str + "'" ) );
    }

    return ret;
}


LoggingProgramOptions::LoggingProgramOptions(
        const std::string& default_logger_name
    ) : _default_logger_name(default_logger_name)
{
    // Nothing to do.
}


void LoggingProgramOptions::addTo(
        boost::program_options::options_description& opts_desc
    )
{
    namespace po = boost::program_options;

    shared_ptr<po::option_description> opt_desc_ptr(
            new po::option_description(
                    "verbose",
                    po::value<Strings>()
                        ->notifier( bind( &LoggingProgramOptions::notifier, this, _1 ) ),
                    "Logging configuration"
                )
        );

    opts_desc.add( opt_desc_ptr );
}


void LoggingProgramOptions::notifier( const Strings& strs )
{
    for ( Strings::const_iterator i(strs.begin()) ; i != strs.end() ; ++i )
    {
        const string &str(*i);

        if ( debug )  std::cerr << __FUNCTION__ << " called with '" << str << "'\n";

        string logger_name;
        LevelPtr level_ptr;

        _parseVerboseString( str, logger_name, level_ptr );

        if ( debug )  std::cerr << __FUNCTION__ << " -> '" << logger_name << "'='" << (level_ptr ? level_ptr->toString() : "null") << "'\n";

        _logging_levels[logger_name] = level_ptr;
    }
}


void LoggingProgramOptions::apply() const
{
    // Go through the stored logging settings and apply to the current logging configuration.

    for ( LoggingLevels::const_iterator i(_logging_levels.begin()) ; i != _logging_levels.end() ; ++i ) {
        Logger::getLogger( i->first )->setLevel( i->second );
    }


    // Log current logger settings.

    log4cxx::LoggerList loggers(log4cxx::Logger::getRootLogger()->getLoggerRepository()->getCurrentLoggers());

    for ( log4cxx::LoggerList::const_iterator i(loggers.begin()) ; i != loggers.end() ; ++i ) {
        if ( (*i)->getLevel() ) {
            LOG_DEBUG_MSG( (*i)->getName() << "=" << (*i)->getLevel()->toString() );
        }
    }
}


void LoggingProgramOptions::_parseVerboseString(
        const std::string& str,
        std::string& logger_name_out,
        log4cxx::LevelPtr& level_ptr_out
    )
{
    if ( str.empty() ) {
        logger_name_out = _default_logger_name;
        level_ptr_out = Level::getDebug();
        return;
    }

    string::size_type split_pos(str.find( '=' ));

    if ( split_pos == string::npos ) { // didn't find =, so is either a logger name or a level.
        if ( str.find( '.' ) == string::npos ) { // no . so it's a level.
            logger_name_out = _default_logger_name;
            level_ptr_out = parseVerboseArgument( str );
            return;
        }

        // found a . so it's a logger name.
        logger_name_out = str;
        level_ptr_out = Level::getDebug();
        return;
    }

    // found = so it's a logger name and level.

    logger_name_out = str.substr( 0, split_pos );
    level_ptr_out = parseVerboseArgument( str.substr( split_pos + 1 ) );
}


} // namespace bgq::utility
} // namespace bgq
