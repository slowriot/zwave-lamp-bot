#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "06";
	static const char MONTH[] = "04";
	static const char YEAR[] = "2019";
	static const char UBUNTU_VERSION_STYLE[] =  "19.04";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 0;
	static const long BUILD  = 79;
	static const long REVISION  = 443;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 95;
	#define RC_FILEVERSION 1,0,79,443
	#define RC_FILEVERSION_STRING "1, 0, 79, 443\0"
	static const char FULLVERSION_STRING [] = "1.0.79.443";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 79;
	

}
#endif //VERSION_H
