#include "precompiled.h"
#include "stuff.h"

std::map<string,string> FileCache::db;

string FileCache::get(string filename) {
	if(db.find(filename)==db.end()) {
		std::vector<unsigned char> buffer;
		loadFile(buffer,filename);
		string bufferStr(&buffer[0],&buffer[buffer.size()]);
		db[filename]=bufferStr;
	}
	return db[filename];
}