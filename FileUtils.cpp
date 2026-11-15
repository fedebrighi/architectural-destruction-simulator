#include "FileUtils.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

std::string FileUtils::getDirectory(const std::string& filepath) {
    size_t found = filepath.find_last_of("/\\");
    if (found != std::string::npos) {
        return filepath.substr(0, found);
    }
    return "";
}

std::string FileUtils::getFilename(const std::string& filepath) {
    size_t found = filepath.find_last_of("/\\");
    if (found != std::string::npos) {
        return filepath.substr(found + 1);
    }
    return filepath;
}

std::string FileUtils::getExtension(const std::string& filepath) {
    size_t found = filepath.find_last_of(".");
    if (found != std::string::npos) {
        return filepath.substr(found);
    }
    return "";
}

std::string FileUtils::combinePath(const std::string& directory, const std::string& filename) {
    if (directory.empty()) return filename;
    if (directory.back() == '/' || directory.back() == '\\') {
        return directory + filename;
    }
    return directory + "/" + filename;
}

bool FileUtils::fileExists(const std::string& filepath) {
    try {
        return fs::exists(filepath) && fs::is_regular_file(filepath);
    }
    catch (const fs::filesystem_error& ex) {
        std::cerr << "FileUtils::fileExists error: " << ex.what() << std::endl;
        return false;
    }
}

bool FileUtils::directoryExists(const std::string& dirpath) {
    try {
        return fs::exists(dirpath) && fs::is_directory(dirpath);
    }
    catch (const fs::filesystem_error& ex) {
        std::cerr << "FileUtils::directoryExists error: " << ex.what() << std::endl;
        return false;
    }
}

std::vector<std::string> FileUtils::getFilesInDirectory(const std::string& directory, const std::string& extension) {
    std::vector<std::string> files;

    try {
        if (!directoryExists(directory)) {
            std::cerr << "Directory does not exist: " << directory << std::endl;
            return files;
        }

        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string filepath = entry.path().string();
                if (extension.empty() || getExtension(filepath) == extension) {
                    files.push_back(filepath);
                }
            }
        }
    }
    catch (const fs::filesystem_error& ex) {
        std::cerr << "FileUtils::getFilesInDirectory error: " << ex.what() << std::endl;
    }

    return files;
}

std::string FileUtils::findModelFile(const std::string& directory, const std::vector<std::string>& extensions) {
    try {
        if (!directoryExists(directory)) {
            std::cerr << "Directory does not exist: " << directory << std::endl;
            return "";
        }

        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string filepath = entry.path().string();
                std::string ext = getExtension(filepath);

                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                for (const std::string& validExt : extensions) {
                    if (ext == validExt) {
                        std::cout << "Found model file: " << filepath << std::endl;
                        return filepath;
                    }
                }
            }
        }
    }
    catch (const fs::filesystem_error& ex) {
        std::cerr << "FileUtils::findModelFile error: " << ex.what() << std::endl;
    }

    std::cerr << "No model file found in directory: " << directory << std::endl;
    return "";
}

std::vector<std::string> FileUtils::findAllModelFiles(const std::string& directory) {
    std::vector<std::string> modelFiles;
    std::vector<std::string> extensions = { ".obj", ".fbx", ".dae", ".3ds", ".blend", ".ply", ".stl" };

    try {
        if (!directoryExists(directory)) {
            std::cerr << "Directory does not exist: " << directory << std::endl;
            return modelFiles;
        }

        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string filepath = entry.path().string();
                std::string ext = getExtension(filepath);

                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                for (const std::string& validExt : extensions) {
                    if (ext == validExt) {
                        modelFiles.push_back(filepath);
                        break;
                    }
                }
            }
        }
    }
    catch (const fs::filesystem_error& ex) {
        std::cerr << "FileUtils::findAllModelFiles error: " << ex.what() << std::endl;
    }

    std::cout << "Found " << modelFiles.size() << " model files in " << directory << std::endl;
    return modelFiles;
}

void FileUtils::printDirectoryContents(const std::string& directory) {
    std::cout << "Contents of directory: " << directory << std::endl;

    try {
        if (!directoryExists(directory)) {
            std::cout << "  Directory does not exist!" << std::endl;
            return;
        }

        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::cout << "  FILE: " << entry.path().filename().string() << std::endl;
            }
            else if (entry.is_directory()) {
                std::cout << "  DIR:  " << entry.path().filename().string() << "/" << std::endl;
            }
        }
    }
    catch (const fs::filesystem_error& ex) {
        std::cerr << "FileUtils::printDirectoryContents error: " << ex.what() << std::endl;
    }
}