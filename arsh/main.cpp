//
//  main.cpp
//  arsh
//
//  Created by Aaron Rennow on 9/5/19.
//  Copyright © 2019 Lithium Cube. All rights reserved.
//

#include <dirent.h>
#include <string_view>
#include <string>
#include <functional>
#include <sys/stat.h>
#include <libgen.h>
#include <unistd.h>

static int recursivelyFindFile(std::string const & filename, std::string const & directoryPath, std::function<bool(std::string const &)> const & matchFunction) {
	int count = 0;
	
	DIR* dir = opendir(directoryPath.c_str());
	if (dir) {
		for (struct dirent* ent; (ent = readdir(dir));) {
			std::string_view nodeName(ent->d_name, ent->d_namlen);
			
			if (nodeName == "." || nodeName == "..") { continue; }
			
			if (ent->d_type == DT_DIR) {
				std::string const subdirectoryPath = directoryPath + "/" + std::string(nodeName);
				count += recursivelyFindFile(filename, subdirectoryPath, matchFunction);
			} else if (nodeName == filename) {
				std::string fullPath;
				fullPath.reserve(directoryPath.length() + filename.length() + 1);
				fullPath += directoryPath;
				fullPath += "/";
				fullPath += filename;
				
				count += 1;
				matchFunction(fullPath);
			}
		}
		closedir(dir);
	} else {
		perror(directoryPath.c_str());
	}
	
	return count;
}

static bool componentsOfFilePath(std::string_view const & filePath, std::string& directory, std::string& filename) {
	auto correctedCPath = const_cast<char*>(filePath.begin());
	directory = dirname(correctedCPath);
	filename = basename(correctedCPath);
	return (!directory.empty() && !filename.empty());
}

int main(int argc, const char * argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Wrong number of parameters; got %i instead of 1\n", argc-1);
		return 1;
	}
	
	int count = recursivelyFindFile("update.ar.sh", argv[1], [](std::string const & path) -> bool {
		struct stat nodeInfo;
		if (stat(path.c_str(), &nodeInfo) == -1) {
			perror("stat");
			exit(1);
		}
		if (!(nodeInfo.st_mode & S_IXUSR)) {
			fprintf(stderr, "%s not executable\n", path.c_str());
			return true;
		}
		
		std::string directory, filename;
		assert(componentsOfFilePath(path, directory, filename));
		
		std::string const command = "cd " + directory + "; ./" + filename;
		
		system(command.c_str());
		return true;
	});
	
	fprintf(stderr, "Found %i files\n", count);
	
	return 0;
}
