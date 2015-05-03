// Copyright (c) 2014-2015 John Biddiscombe
// Copyright (c) 2011,2012 IBM Corp.
//
// ================================================================
// Portions of this code taken from IBM BlueGene-Q source
// 
// This software is available to you under the
// Eclipse Public License (EPL).
//
// Please refer to the file "eclipse-1.0.txt" 
// ================================================================
//
#include "RdmaLogging.h"

#ifdef RDMAHELPER_HAVE_LOGGING

//#include <boost/format.hpp>
    #include <boost/log/trivial.hpp>
    #include <boost/log/expressions/formatter.hpp>
    #include <boost/log/expressions/formatters.hpp>
    #include <boost/log/expressions/formatters/stream.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/utility/manipulators/to_log.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

void initRdmaHelperLogging()
{
    std::cout << "Initializing logging " << std::endl;
    logging::add_console_log(
    std::clog,
    // This makes the sink to write log records that look like this:
    // 1: <normal> A normal severity message
    // 2: <error> An error severity message
    keywords::format =
        (
            expr::stream
            //            << (boost::format("%05d") % expr::attr< unsigned int >("LineID"))
            << expr::attr< unsigned int >("LineID")
            << ": <" << logging::trivial::severity
            << "> " << expr::smessage
        )
    );
    logging::add_common_attributes();
}

#endif

