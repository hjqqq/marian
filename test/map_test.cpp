#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE MapTest
#include <boost/test/unit_test.hpp>
#include "TmxMap.h"

BOOST_AUTO_TEST_CASE(map_test)
{
  tmx::Map map;
  map.loadFromFile("../media/map1.tmx");

  BOOST_REQUIRE( map.width == 20 );
  BOOST_REQUIRE( map.height == 20 );
  BOOST_REQUIRE( map.tileWidth == 32 );
  BOOST_REQUIRE( map.tileHeight == 32 );

  BOOST_TEST_MESSAGE( "Layers size == " << map.layers.size() );
  BOOST_REQUIRE( map.layers.size() == 4 );

  BOOST_REQUIRE( map.objectGroups.size() == 1 );
  BOOST_REQUIRE( map.objectGroups[0].objects.size() == 1 );
}
