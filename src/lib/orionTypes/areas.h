#ifndef SRC_LIB_ORIONTYPES_AREAS_H_
#define SRC_LIB_ORIONTYPES_AREAS_H_

/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include <stdlib.h>    // atof
#include <string>
#include <vector>



namespace orion
{
/* ****************************************************************************
*
* AreaType -
*/
typedef enum AreaType
{
  NoArea,
  CircleType,
  PolygonType
} AreaType;



/* ****************************************************************************
*
* Point -
*/
class Point
{
 private:
  ::std::string _latitude;
  ::std::string _longitude;

 public:
  Point();
  Point(::std::string latitude, ::std::string longitude);

  void   fill(Point* p);
  double latitude(void) const;
  double longitude(void) const;
  void   latitudeSet(::std::string latitude);
  void   longitudeSet(::std::string longitude);
  ::std::string latitudeString(void);
  ::std::string longitudeString(void);
};



/* ****************************************************************************
*
* Circle -
*/
class Circle
{
 private:
  ::std::string  _radius;
  ::std::string  _inverted;

 public:
  Point          center;
  bool           inverted(void) const;
  double         radius(void) const;

  ::std::string  radiusString(void) const;
  ::std::string  invertedString(void) const;
  void           radiusSet(::std::string radius);
  void           radiusSet(float _radius);
  void           invertedSet(::std::string inverted);
  void           invertedSet(bool _inverted);
  void           centerSet(Point* _center);
};



/* ****************************************************************************
*
* Polygon -
*/
class Polygon
{
 private:
  ::std::string         _inverted;

 public:
  ::std::vector<Point*> vertexList;
  bool                  inverted(void) const;
  void                  invertedSet(::std::string inverted);
  void                  invertedSet(bool inverted);
  ::std::string         invertedString(void) const;
  void                  vertexAdd(Point* p);
  void                  release(void);
};



/* ****************************************************************************
*
* Geometry - 
*/
class Geometry
{
public:
  Geometry();
  int          parse(const char* in, std::string* errorString);

  std::string  areaType;
  float        radius;
  bool         external;
};

}

#endif  // SRC_LIB_ORIONTYPES_AREAS_H_
