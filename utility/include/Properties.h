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
 * \file utility/include/Properties.h
 * \brief \link bgq::utility::Properties Properties\endlink definition.
 */

#ifndef BGQ_UTILITY_PROPERTIES_H
#define BGQ_UTILITY_PROPERTIES_H

#include <boost/thread/shared_mutex.hpp>

#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <inttypes.h>


namespace bgq {
namespace utility {

/*!
 * \brief The bg.properties configuration file
 *
 * \section properties_overview Overview
 *
 * The properties file is the single central location for all Blue Gene/Q software configuration
 * information. It replaces several configuration files from previous generation Blue Gene systems
 * such as db.properties, bridge.config, and mpirun.cfg. It is the central location to configure
 * database connection information, configure and control logging behavior, as well as the configuration 
 * of every component present in the Blue Gene/Q software stack. The location of this file is
 *
 * \code
 * /bgsys/local/etc/bg.properties
 * \endcode
 *
 * It contains no plain text passwords and should be readable by all users.
 *
 * \section Format
 *
 * The format expected by this file is similar to an INI file. Sections are enclosed in square
 * brackets. Sub-sections have one or more periods in the section name.
 *
 * \code
 *
 * [section]
 * key=value
 * foo=bar
 *
 * [another section]
 * key=value
 * bar=foo
 *
 * \endcode
 *
 * Leading and trailing spaces around the section name, key, and value
 * are trimmed.  Comments can start anywhere in a line and are denoted
 * by a single pound # character.
 *
 * \subsection comments Comment Style
 *
 * In order to aid readability, the comment style is uniform for every section in the file.
 *
 * - Two blank lines before a [section] name.
 * - One blank line after a [section] name.
 * - One blank line after each key = value pair with a comment.
 * - An optional comment after a [section] name describing the section.
 * - One space before the equals character and one after of a key = value pair.
 * - Keys do not need values after the equals character.
 * - Comments that are complete sentences begin with a capital letter and
 *   end with a period.
 *
 * Example:
 *
 * \code
 *
 * [Widget]
 *     # Configuration options for the widget server.
 *
 * size = 10
 *     # The number of widget factories to instantiate at startup.
 *     # If not specified, the default value is 5.
 *
 * threads =
 *     # The number of worker threads to start.
 *     # 0 indicates to let the widget server pick, otherwise it must
 *     # be a positive number.
 *
 * uid = root
 * group = root
 *     # Run the widget server as the specified user and group.
 *     # If not specified, it runs as root.
 *
 * \endcode
 *
 * \section contents File Contents
 *
 * The contents of the bg.properties file contains configuration settings for the entire Blue Gene/Q
 * software stack. An exhaustive description of the file is outside the scope of this chapter, consult
 * the specific component chapter or section for a description of its configuration information.
 *
 * \subsection defaults Default Values
 *
 * Certain key values will have component specific defaults that will be used if the value is not present
 * in the file. These will be indicated as a comment like so
 *
 * \code
 * size = 10
 *     # The number of threads to start, must be a positive number.
 *     # If not specific, 5 will be used.
 *
 * \endcode
 * 
 * Similarly, if the value has a specific range, the comment will indicate so.
 *
 * \section API
 *
 * The Properties class is an API to parse the bg.properties file.
 *
 * Duplicate keys in a single section, or duplicate sections in a file
 * result in an exception being thrown.
 *
 * \subsection Debug
 *
 * Set the BG_PROPERTIES_DEBUG environment variable to print trace log
 * messages before logging is configured.
 *
 * \subsection Threading
 *
 * A single instance of this class is thread safe. It is protected by
 * a readers/writer lock to allow multiple calls to Properties::getValue
 * Properties::getValues and Properties::reload.
 *
 */
class Properties : private boost::noncopyable
{
public:
    /*!
     * \brief Name of environmental used to read properties file if a path is not provided.
     */
    static const std::string EnvironmentalName;

    /*!
     * \brief Default path of properties file if one is not provided.
     */
    static const std::string DefaultLocation;

public:
    typedef std::pair<std::string, std::string> Pair;       //!< key=value pair
    typedef std::vector<Pair> Section;                      //!< container of Pairs
    typedef std::map<std::string, Section> Map;             //!< mapping of names to Sections
    typedef boost::shared_ptr<Properties> Ptr;              //!< pointer type
    typedef boost::shared_ptr<const Properties> ConstPtr;   //!< const pointer type

    /*
     * \brief Properties file cannot be found or opened.
     */
    struct FileError : public std::runtime_error
    {
        FileError(const std::string& what) : std::runtime_error(what) { }
    };

    /*!
     * \brief [section] contains a duplicate key.
     */
    struct DuplicateKey : public std::runtime_error
    {
        DuplicateKey(const std::string& what) : std::runtime_error(what) { }
    };

    /*!
     * \brief file contains a duplicate section.
     */
    struct DuplicateSection : public std::runtime_error
    {
        DuplicateSection(const std::string& what) : std::runtime_error(what) { }
    };

    /*!
     * \brief line is missing an equals character
     */
    struct MalformedKey : public std::runtime_error
    {
        MalformedKey(const std::string& what) : std::runtime_error(what) { }
    };

    /*!
     * \brief line is missing a left or right bracket or both
     */
    struct MalformedSection : public std::runtime_error
    {
        MalformedSection(const std::string& what) : std::runtime_error(what) { }
    };

    /*!
     * \brief key=value found before a section
     */
    struct MissingSection : public std::runtime_error
    {
        MissingSection(const std::string& what) : std::runtime_error(what) { }
    };


    /*!
     * \brief Standard program options descriptions.
     *
     * The standard program option name is --properties.
     *
     */
    class ProgramOptions {
    public:
        /*!
         *  \brief Add the standard program options descriptions to the options description.
         *
         *  Adds an option description for --properties that sets _filename.
         */
        void addTo(
                boost::program_options::options_description& opts_desc //!< [inout]
                );

        /*!
         * \brief Gets the filename.
         *
         * Call this after boost::program_options::notify. Pass the result to the Properties::Properties()
         */
        const std::string& getFilename() const  { return _filename; }

    private:
        std::string _filename;
    };


public:
    /*!
     * \brief ctor.
     *
     * Empty file argument means use EnvironmentalName environment variable, or if it's not set, the
     * DefaultLocation.
     *
     * \throws FileError
     * \throws DuplicateKey
     * \throws DuplicateSection
     * \throws MalformedKey
     * \throws MalformedSection
     * \throws MissingSection
     */
    explicit Properties(
            const std::string& file = std::string()     //!< [in] file to open
            );

    /*!
     * \copydoc Properties::Properties()
     */
    static Ptr create(
            const std::string& file = std::string()     //!< [in] file to open
            )
    {
        return Ptr( new Properties(file) );
    }

    /*!
     * \brief get the properties file name.
     */
    const std::string& getFilename() const { return _filename; }

    /*!
     * \brief get a value from the config file contents.
     *
     * \throws std::invalid_argument if the section  cannot be found
     */
    const std::string& getValue(
            const std::string& section,     //!< [in] section name
            const std::string& key          //!< [in] key name
            ) const;

    /*!
     * \brief get all values for a given section.
     *
     * \throws std::invalid_argument if the section cannot be found.
     */
    const Section& getValues(
            const std::string& section      //!< [in] section name
            ) const;

    /*!
     * \brief reloads the config file.
     *
     * \throws std::invalid_argument if filename is not fully qualified
     * \throws std::exception if reloading fails
     *
     * \returns true on success, false otherwise
     *
     * \post The internal Map is unchanged if reloading fails.
     */
    bool reload(
            const std::string& filename = std::string() //!< [in]
            );

private:
    /*!
     * \brief read config file.
     *
     * \throws MalformedSection or MalformedKey if a malformed line is found
     * \throws DuplicateSection if a duplicate section is found
     * \throws DuplicateKey if a duplicate key is found within a section
     */
    void read();

    /*!
     * \brief parse a line looking for a [section] indicator.
     *
     * \throws DuplicateSection if the section already exists
     * \throws MalformedSection if the section is missing either bracket
     */
    Map::iterator parseSection(
            const std::string& name,    //!< [in] line text
            uint32_t lineno             //!< [in] line number
            );

    /*!
     * \brief parse a line looking for a key=value pair.
     *
     * \throws DuplicateKey if a duplicate key is found
     * \throws MalformedKey if a line is missing an equals token
     *
     * \post the key=value pair is added to the Section specified by section
     */
    void parseLine(
            Map::iterator section,      //!< [in]
            const std::string& line,    //!< [in]
            uint32_t lineno             //!< [in]
            );

    /*!
     * \brief
     */
    const std::string& getValueImpl(
            const std::string& section,     //!< [in] section name
            const std::string& key          //!< [in] key name
            ) const;

    /*!
     * \brief
     */
    const Section& getValuesImpl(
            const std::string& section      //!< [in] section name
            ) const;
private:
    typedef boost::shared_mutex Mutex;

private:
    std::string _filename;              //!< properties file name
    Map _map;                           //!< key/value pairs
    mutable Mutex _mutex;               //!< reader/writer lock to protect map
};

} // utility
} // bgq

#endif
