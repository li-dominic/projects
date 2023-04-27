#include <unistd.h>
#include <stdio.h>
#include <stdio.h>
#include <ctype.h>
#include <string>
#include <vector>

unsigned char buffer [1024*1024]; // size of buffer 1 MB - try to read this number of characters from file from every read call
int bufferSize = 0; // this will be used to track how many characters were successfully read and put into buffer
int bufferIndex = 0; // used to track where in the buffer we are to send correct character, and also when we run out of new characters to send

std::string get_longest_palindrome();
bool is_palindrome (const std::string &s);
std::vector<std::string> split(const std::string & p_line);
std::string stdin_readline();
char readOneChar();

int main()
{
  std::string max_pali = get_longest_palindrome();
  printf("Longest palindrome: %s\n", max_pali.c_str());
  return 0;
}

std::string get_longest_palindrome()
{
  std::string max_pali;
  while(1) {
    std::string line = stdin_readline();
    if (line.size() == 0) 
      break;
    auto words = split(line);
    for (auto word : words) {
      if (word.size() <= max_pali.size())
        continue;
      if (is_palindrome(word))
        max_pali = word;
    }
  }
  return max_pali;
}

bool is_palindrome (const std::string &s)
{
  for (size_t i = 0 ; i < s.size() / 2 ; i ++)
    if (tolower(s[i]) != tolower(s[s.size()-i-1]))
      return false;
  return true;
}

std::string stdin_readline() 
 {
  std::string result; // some of the below code was taken from fast-int.cpp in https://gitlab.com/cpsc457w23/longest-int.git from Professor Pavol Federl 
  while (true) // keep reading and adding to result until EOF or newline, even whitespaces as these will get removed at split() function
  {
    char c = readOneChar(); // call function to retrieve next character
    if (c == -1) // if the file is empty - no more lines to read
    {
      break;
    }
    if (c == '\n')  // if the next character is a newline - it means end of word, return result if it is not empty (do not add to result)
    {
      if (result.size() > 0) 
      {
	      break;
      }
    }	    
    else
    {
      result.push_back(c); // otherwise, file not empty, add character to string
    }
  }

  return result;
}

char readOneChar() // this function was taken from fast-int.cpp in https://gitlab.com/cpsc457w23/longest-int.git from Professor Pavol Federl
{
  if (bufferIndex >= bufferSize) // if buffer is empty (the buffer index reached end of buffer) call read to fill up buffer again
  {
    bufferSize = read(STDIN_FILENO, buffer, sizeof(buffer)); // read 1 MB characters into buffer
    
    if (bufferSize <= 0) // read failed, end of file reached - nothing else to read, return -1
    {
      return -1;
    }
    bufferIndex = 0; // otherwise read successful, reset the index
  }
  
  return buffer[bufferIndex++]; // return the current character in buffer, then iterate index
}

std::vector<std::string> split(const std::string & p_line)
{
  auto line = p_line + " ";
  bool in_str = false;
  std::string curr_word = "";
  std::vector<std::string> res;
  for (auto c : line) 
  {
    if (isspace(c)) 
    {
      if (in_str)
        res.push_back(curr_word);
      in_str = false;
      curr_word = "";
    }
    else 
    {
      curr_word.push_back(c);
      in_str = true;
    }
  }
  return res;
}
