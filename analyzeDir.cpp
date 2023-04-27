#include "analyzeDir.h"
#include <cassert>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <unordered_map>
#include <iostream>
#include <algorithm>
#include <set>

std::unordered_map<std::string, int> histogram;
std::vector<std::pair<std::string, std::pair<long, long> > > imgD;
FILE *fileP;

/// check if path refers to a directory
static bool is_dir(const std::string & path)
{
  struct stat buff;
  if (0 != stat(path.c_str(), &buff)) return false;
  return S_ISDIR(buff.st_mode);
}

/// check if path refers to a file
static bool is_file(const std::string & path)
{
  struct stat buff;
  if (0 != stat(path.c_str(), &buff)) return false;
  return S_ISREG(buff.st_mode);
}

/// check if string ends with another string
static bool ends_with(const std::string & str, const std::string & suffix)
{
  if (str.size() < suffix.size()) return false;
  else
    return 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

/// check if string starts with another string
static bool starts_with(const std::string & str, const std::string & suffix) // taken from Professor Federl code (main.cpp) [https://gitlab.com/cpsc457/public/findLargestDir/-/tree/main/]
{
  if (str.size() < suffix.size()) return false;
  else
    return 0 == str.compare(0, suffix.size(), suffix);
}

static std::string next_word() // this function was taken and derived from Professor Federl's code in main.cpp at [https://gitlab.com/cpsc457/public/word-histogram/-/blob/master/main.cpp]
{
  std::string result;
  while(1) {
    int c = fgetc(fileP);
    if(c == EOF) break;
    c = tolower(c);
    if(! isalpha(c)) { // if character is not in the alphabet, and result is not empty, return it, otherwise continue in the loop
      if(result.size() < 5) // if it finds a non-alphabet character, and there are less than 5 letters in result, then reset it to empty string, and don't add the character c
      {
        result = "";
        continue;
      }
      else{ // otherwise, result contains 5 or more letters so we can return it at this point
        break;
      }
    } 
    result.push_back(c);
  }
  
  return result;
}

long convert2Int(std::string &img_size, int wh) // takes in two arguments, the image size string, and wh which is just a flag to determine which dimension to return
{
  long w; // used to represent the width of the image
  long h; // used to represent the height of the image
  sscanf(img_size.c_str(), "%ld %ld", &w, &h); // this line was taken from Professor Federl (main.cpp) [https://replit.com/@pfederl/str22ints]
  if (wh == 0) return w; // if parameter wh is 0, then want to return width (w)
  else return h; // otherwise for other values return height of image (h)
}

static bool myCompare(std::pair<std::string, std::pair<long, long>> p1, std::pair<std::string, std::pair<long, long>> p2) // this function was derived from Xining's tutorial (sortClassList.cpp)
{
  return ((p1.second.first*p1.second.second) > (p2.second.first*p2.second.second));  // want to sort by ascending width*height value as defined in assignment document
}

static std::vector<ImageInfo> compareIMG(int n) // this function uses the global variable imgD which contains all images from recursion and sorts it into an n long vector 
{
  std::vector<ImageInfo> result;

  if (imgD.size() > size_t(n)) // if imgD is larger than n, then only have to sort n first entries
  {
    std::partial_sort(imgD.begin(), imgD.begin() + n, imgD.end(), myCompare);
    imgD.resize(n);
  }
  else std:: sort(imgD.begin(), imgD.end(), myCompare); // otherwise, sort the entire array, as it is <= n

  for (auto &e : imgD)
  {
    ImageInfo temp = {e.first, e.second.first, e.second.second}; // declare and initialize a temp ImageInfo to insert it in the result to be returned
    result.emplace_back(temp); // append temp to result 
  } 

  return result;
} 

static Results scan(const std::string &dir) // this is the recursive function that will obtain required data from all directories/files
{
  Results list = {"", -1, 0, 0,0}; // variable to be returned, as stated in the assignmnet document, largest_file_size is defaulted to zero, as well as largest_file_path to empty string
  std::string directory; // this variable contains the original dir but without the leading ./
  if(starts_with(dir, "./")) directory = dir.substr(2).c_str(); // if dir starts with ./ remove it and assign to directory variable
  else directory = dir.c_str(); // otherwise, it does not start with ./, assign to directory var

  std::vector<std::string> subdirs; // this contains the empty subdirectories, to be used to find top level vacant directories
  std::string pName; // this will containt the same string as the "path" variable in the loop, but without the leading ./ (if there is)

  auto dirp = opendir(dir.c_str()); // taken from Professor Federl code (main.cpp) [https://gitlab.com/cpsc457/public/findLargestDir/-/tree/main/]
  assert(dirp != nullptr); // taken from Professor Federl code (main.cpp) [https://gitlab.com/cpsc457/public/findLargestDir/-/tree/main/]
  for (auto de = readdir(dirp); de != nullptr; de = readdir(dirp)) { // taken from Professor Federl code (main.cpp) [https://gitlab.com/cpsc457/public/findLargestDir/-/tree/main/]
    std::string name = de->d_name; // taken from Professor Federl code (main.cpp) [https://gitlab.com/cpsc457/public/findLargestDir/-/tree/main/]
    if (name == "." or name == "..") continue; // taken from Professor Federl code (main.cpp) [https://gitlab.com/cpsc457/public/findLargestDir/-/tree/main/]
    std::string path = dir + "/" + name; // taken from Professor Federl code (main.cpp) [https://gitlab.com/cpsc457/public/findLargestDir/-/tree/main/]
    if (starts_with(path, "./")) pName = path.substr(2).c_str(); // if path starts with ./, remove it and assign to pName
    else pName = path.c_str(); // else it does not, assign to pName
    if (is_file(path)) // taken from Professor Federl code (main.cpp) [https://gitlab.com/cpsc457/public/findLargestDir/-/tree/main/]
    {
      list.n_files++; // if the path name is actually a file, then increment the n_files variable, and check to see if it is an image first, and check to see if it is the largest file found thus far
      struct stat statBuffer; // use struct stat to determine file size, the following 3 lines of code is derived from the original analyzeDir.cpp function given and its use of stat to determine file size given from Professor Federl
      stat(path.c_str(), &statBuffer);
      int fsize = statBuffer.st_size;
      list.all_files_size += fsize; // add the file size to the all_file_size variable
      if (fsize > list.largest_file_size) // check to see if this file is larger than what is stored currently, if so then replace the size and path to the file
      {
        list.largest_file_size = fsize;
        list.largest_file_path = pName; // use pName because it does not have the leading "./"
      }
      std::string cmd = "identify -format '%w %h' " + path + " 2> /dev/null"; // this code was taken from the original analyzeDir.cpp given from Professor Federl
      std::string img_size;
      auto fp = popen(cmd.c_str(), "r"); // this code was taken from the original analyzeDir.cpp given from Professor Federl
      assert(fp != nullptr); // this code was taken from the original analyzeDir.cpp given from Professor Federl
      char buff[PATH_MAX]; // this code was taken from the original analyzeDir.cpp given from Professor Federl
      if (fgets(buff, PATH_MAX, fp) != NULL) img_size = buff; // this code was taken from the original analyzeDir.cpp given from Professor Federl
      int status = pclose(fp); // this code was taken from the original analyzeDir.cpp given from Professor Federl
      if (!status) // if pclose() returns 0, then it is an image, then use convert2Int to get the dimensions of the image
      {
        long w = convert2Int(img_size, 0); // get width of image
        long h = convert2Int(img_size, 1); // get height of image
        imgD.push_back(std::make_pair(pName, std::make_pair(w, h))); // use global variable containing two pairs to append the image name, width, and height
      }
      else if (ends_with(path, ".txt")) // otherwise, if file is not an image, check to see if it is a text file.
      {
        fileP = fopen(pName.c_str(), "r"); // use fopen() to open the file 
        while(1) // taken from Professor Federl code (main.cpp) [https://gitlab.com/cpsc457/public/word-histogram/-/blob/master/main.cpp]
        {
          auto w = next_word(); // taken from Professor Federl code (main.cpp) [https://gitlab.com/cpsc457/public/word-histogram/-/blob/master/main.cpp]
          if (w.size() == 0) break; // taken from Professor Federl code (main.cpp) [https://gitlab.com/cpsc457/public/word-histogram/-/blob/master/main.cpp]
          else histogram[w] ++; // derived from Professor Federl code (main.cpp) [https://gitlab.com/cpsc457/public/word-histogram/-/blob/master/main.cpp], only want words with 5 letters or more
        }
        fclose(fileP); // close the file after reading all data from it
      }
    }
    else if(is_dir(path)) // taken from Professor Federl code (main.cpp) [https://gitlab.com/cpsc457/public/findLargestDir/-/tree/main/]
    {
      Results temp = scan(path); // if it is a path, then call scan() again recursively on that directory
      list.n_dirs += temp.n_dirs + 1; // add the number of directories, plus 1 because it just found a directory 
      list.n_files += temp.n_files; // add number of files and total file sizes from recursive call
      list.all_files_size += temp.all_files_size;
      if (temp.largest_file_size > list.largest_file_size) // check to see if recursion found a larger file, if so, replace it
      {
        list.largest_file_size = temp.largest_file_size;
        list.largest_file_path = temp.largest_file_path;
      }
      subdirs.insert(subdirs.end(), temp.vacant_dirs.begin(), temp.vacant_dirs.end()); // append the vacant subdirectories to the subdirs variable
    } 
  }
  closedir(dirp); // close the open directory

  if (list.n_files == 0) // check to see if the total number of files in this directory is 0, if so, append this directory to the list of vacant dirs
  {
    list.vacant_dirs.push_back(directory.c_str()); 
  }
  else list.vacant_dirs.insert(list.vacant_dirs.end(), subdirs.begin(), subdirs.end()); // if there are files in THIS directory, append the empty subdirectories if any

  return list; // return the result of this function call in list
}

Results analyzeDir(int n) 
{
  Results res;
 
  Results resultN;
  resultN = scan("."); // call recursive function on current directory from user input

  res.n_files = resultN.n_files; // assign the result to the returning Results variable res
  res.vacant_dirs = resultN.vacant_dirs; 
  res.n_dirs = resultN.n_dirs + 1; // add 1 to include current directory (".")
  res.all_files_size = resultN.all_files_size;
  res.largest_file_path = resultN.largest_file_path;
  res.largest_file_size = resultN.largest_file_size;
  
  std::vector<std::pair<int, std::string>> arr; // this code was taken from Professor Federl (main.cpp) [https://gitlab.com/cpsc457/public/word-histogram/-/blob/master/main.cpp]
  for (auto &h : histogram) // this code was taken from Professor Federl (main.cpp) [https://gitlab.com/cpsc457/public/word-histogram/-/blob/master/main.cpp]
    arr.emplace_back(-h.second, h.first); // this code was taken from Professor Federl (main.cpp) [https://gitlab.com/cpsc457/public/word-histogram/-/blob/master/main.cpp]

  if (arr.size() > size_t(n)) // this code was taken from Professor Federl (main.cpp) [https://gitlab.com/cpsc457/public/word-histogram/-/blob/master/main.cpp]
  {
    std::partial_sort(arr.begin(), arr.begin() + n, arr.end()); // this code was taken from Professor Federl (main.cpp) [https://gitlab.com/cpsc457/public/word-histogram/-/blob/master/main.cpp]
    arr.resize(n); // this code was taken from Professor Federl (main.cpp) [https://gitlab.com/cpsc457/public/word-histogram/-/blob/master/main.cpp]
  } else std::sort(arr.begin(), arr.end()); // this code was taken from Professor Federl (main.cpp) [https://gitlab.com/cpsc457/public/word-histogram/-/blob/master/main.cpp]

  for (auto &v : arr) res.most_common_words.emplace_back(v.second, -v.first); // reverse the sorting order
  
  res.largest_images = compareIMG(n); // call function to sort n largest images from global variable
  
  
  return res;
}

