#pragma once

#include <string>
#include <vector>

class FileUtils {
public:

    static std::string getDirectory(const std::string& filepath);
    static std::string getFilename(const std::string& filepath);
    static std::string getExtension(const std::string& filepath);
    static std::string combinePath(const std::string& directory, const std::string& filename);

    static bool fileExists(const std::string& filepath);
    static bool directoryExists(const std::string& dirpath);

    static std::vector<std::string> getFilesInDirectory(const std::string& directory, const std::string& extension = "");


    static std::string findModelFile(const std::string& directory, const std::vector<std::string>& extensions = { ".obj", ".blend" });
    static std::vector<std::string> findAllModelFiles(const std::string& directory);

    static void printDirectoryContents(const std::string& directory);
};