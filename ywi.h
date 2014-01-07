#ifndef __YWI_H__
#define __YWI_H__

#include "ywcommon.h"

#include <string>
#include <list>

bool ywiInsert(
    std::string                                     &key, 
    std::list< std::pair<std::string,std::string> > &values,
    std::list< std::pair<std::string,std::string> > &meta );


bool ywiGet(
    std::string                                     &key, 
    std::list< std::pair<std::string,std::string> > &values,
    std::list< std::pair<std::string,std::string> > &meta );


#endif
