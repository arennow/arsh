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

static void printHelp() {
	enum struct Requirement : uint8_t {
		nothing = 0, taskName
	};
	
	struct {
		char flag;
		std::string description;
		Requirement requirement;
	} const flags[] = {
		{'h', "Print help information"},
		{'v', "Be verbose"},
		{'t', "Only run tasks with provided name", Requirement::taskName}
	};
	
	fputs("arsh ", stdout);
	
	for (auto& flag : flags) {
		fputc('[', stdout);
		printf("-%c", flag.flag);
		
		switch (flag.requirement) {
			case Requirement::taskName:
				fputs(" TASK", stdout);
				break;
				
			case Requirement::nothing:
			default:
				break;
		}
		
		fputs("] ", stdout);
	}
	
	puts("<ScanDir>");
	
	for (auto& flag : flags) {
		printf("   -%c\t%s\n", flag.flag, flag.description.c_str());
	}
}

static inline bool hasEnding(std::string_view fullString, std::string_view ending) {
	if (fullString.length() >= ending.length()) {
		return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
	} else {
		return false;
	}
}

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
			} else {
				if ((filename.empty() && hasEnding(nodeName, ".ar.sh")) || (!filename.empty() && nodeName == filename)) {
					std::string fullPath;
					fullPath.reserve(directoryPath.length() + nodeName.length() + 1);
					fullPath += directoryPath;
					fullPath += "/";
					fullPath += nodeName;
					
					count += 1;
					matchFunction(fullPath);
				}
			}
		}
		closedir(dir);
	} else {
		perror(directoryPath.c_str());
	}
	
	return count;
}

static inline bool componentsOfFilePath(std::string_view const & filePath, std::string& directory, std::string& filename) {
	auto correctedCPath = const_cast<char*>(filePath.begin());
	directory = dirname(correctedCPath);
	filename = basename(correctedCPath);
	return (!directory.empty() && !filename.empty());
}

int main(int argc, char * argv[]) {
	std::string filename;
	std::string directory;
	bool verbose = false;
	
	int ch;
	while ((ch = getopt(argc, argv, "hvt:")) != -1) {
		switch (ch) {
			case 'h':
				printHelp();
				return 0;
				
			case 't':
				filename = optarg;
				filename += ".ar.sh";
				break;
				
			case 'v':
				verbose = true;
				break;
				
			case ':':
				fprintf(stderr, "Unexpected option -%c\n", optopt);
				return 2;
		}
	}
	
	if (argc > optind) {
		directory = argv[optind];
	} else {
		fputs("No scan directory given\n", stderr);
		return 2;
	}
	
	int count = recursivelyFindFile(filename, directory, [](std::string const & path) -> bool {
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
	
	if (verbose) {
		fprintf(stderr, "Found %i files\n", count);
	}
	
	return 0;
}
