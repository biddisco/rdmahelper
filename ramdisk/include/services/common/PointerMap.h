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
/* (C) Copyright IBM Corp.  2011, 2012                              */
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

//! \file  PointerMap.h
//! \brief Declaration and inline methods for bgcios::PointerMap class.

#ifndef COMMON_POINTERMAP_H
#define COMMON_POINTERMAP_H

// Includes
#include <boost/unordered_map.hpp>

namespace bgcios
{

//! Template class for a map where the key is an integer type and the value is a pointer.

template <class K, class V> class PointerMap
{
public:
   //! Container type
   typedef boost::unordered_map<K, V> Container;

   //! Iterator for accessing elements in map.
   typedef typename Container::iterator iterator;

   //! Constant iterator for accessing elements in map.
   typedef typename Container::const_iterator const_iterator; 

   //! \brief  Add an element to the map.
   //! \param  key Key for finding element in map.
   //! \param  value Value of element.
   //! \return Nothing.

   void add(K key, V value)
   {
      // Add the key/value pair to the map.
      _map[key] = value;
      return;
   }

   //! \brief  Remove an element from the map.
   //! \param  key Key to element.
   //! \return Nothing.

   void remove(K key)
   {
      // Locate the key in the map.
      iterator iter = _map.find(key);
      if (iter == _map.end()) {
         return; // Just return if key not found
      }

      // Remove the key/value pair from the map.
      _map.erase(iter);
      return;
   }

   //! \brief  Get value for specified key.
   //! \param  key Key to element.
   //! \return Value of element with specified key.

   const V& get(const K key)
   {
      // Locate the key in the map.
      const const_iterator iter = _map.find(key);
      if (iter == _map.end()) {
         return _notFound;
      }
      return iter->second;
   }

   //! \brief  Check if specified key is in the map.
   //! \param  key Key to element.
   //! \return True if key is in the map, false if key is not in the map.

   bool isValid(K key)
   {
      // Locate the key in the map.
      const_iterator iter = _map.find(key);
      if (iter == _map.end()) {
         return false;
      }
      return true;
   }

   //! \brief  Check if map is empty.
   //! \return True if map is empty, otherwise false.

   bool empty(void) const { return _map.empty(); }

   //! \brief  Clear the map.
   //! \return nothing

   void clear(void) { return _map.clear(); }

   //! \brief  Return number of elements in the map.
   //! \return Number of elements.

   size_t size(void) const { return _map.size(); }

   //! \brief  Return iterator to first element of the map.
   //! \return Iterator to beginning of the map.

   const_iterator begin(void) { return _map.begin(); }

   //! \brief  Return iterator just past last element of the map.
   //! \return Iterator to end of the map.

   const_iterator end(void) { return _map.end(); }

private:

   //! Map of key and value pairs.
   Container _map;

   //! Empty value to indicate key was not found in map.
   V _notFound;

};

} // namespace bgcios

#endif // COMMON_POINTERMAP_H
