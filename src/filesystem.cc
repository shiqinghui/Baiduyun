/*
 * Copyright (c) 2014 loujiayu
 * This file is released under the MIT License
 * http://opensource.org/licenses/MIT
 */
#include <stdio.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "filesystem.h"
#include "jsonentry.h"

namespace by {

DirIter::DirIter() {
  filesystem_iter_ = fs::directory_iterator();
}

DirIter::DirIter(const std::string& p) : path_(p) {
  filesystem_iter_ = fs::directory_iterator(path_);
  if(filesystem_iter_ != fs::directory_iterator())
    UpdatePara();
}

void DirIter::UpdatePara() {
  JsonEntry json;
  std::string file= (*filesystem_iter_).path().string();
  std::ifstream ifile(file.c_str(), std::ios::binary | std::ios::out);
  std::string md5 = MD5(ifile.rdbuf());
  json.Add("path",JsonEntry((*filesystem_iter_).path().string()));
  json.Add("md5",JsonEntry(md5));
  jstring_ = json.getstring();
}

DirIter& DirIter::operator++() {
  ++filesystem_iter_;
  if(filesystem_iter_ == fs::directory_iterator())
    return *this;
  UpdatePara();
  return *this;
}

DirIter DirIter::operator++(int) {
  DirIter tmp(*this);
  operator++();
  return tmp;
}

JsonEntry DirIter::operator*() {
  return JsonEntry::Parse(jstring_);
}

bool IsDir(const JsonEntry& json) {
  return json["isdir"].Value<unsigned int>();
}

std::string ParseFileName(const JsonEntry& json) {
  std::string path = json["path"].Value<std::string>();
  return path;
}

unsigned int ParseFilemTime(const JsonEntry& json) {
  return json["mtime"].Value<unsigned int>();
}

std::string FileFromPath(const std::string& path) {
  auto found = path.find_last_of("/\\");
  return path.substr(found + 1);
}

bool FileSystem::CreatDir(const std::string &path) {
  if(!IsExist(path))
    if(!fs::create_directory(path)){
      printf("Pathname:%s invalid.",path.c_str());
      return false;
    }
  return true;
}

bool FileSystem::IsLocalDir(const std::string &path) {
  return fs::is_directory(path);
}

void FileSystem::GetChild(std::forward_list<JsonEntry> &list, const std::string &path) {
  copy(DirIter(path),DirIter(),front_inserter(list));
}

unsigned int FileSystem::LastWriteTime(const std::string &path) {
  return fs::last_write_time(path);
}

bool FileSystem::IsExist(const std::string &path) {
  return fs::exists(path);
}

bool FileSystem::DeleteFile(const std::string &path) {
  return fs::remove(path);
}

bool FileSystem::DirIsEmpty(const std::string &path) {
  return fs::is_empty(path);
}

bool FileSystem::DeleteDir(const std::string& path) {
  boost::system::error_code ec;
  if (!boost::filesystem::remove_all(path, ec)) {
    std::cout << path << ':' << ec.message();
    return false;
  }
  return true;
}

bool FileSystem::NewFile(File **file,const std::string &fname, int mode) {
  std::string path = "MyBaidu/" + fname;
  FILE *f;
  if(mode > 0)
    f = fopen(path.c_str(),"w");
  else
    f = fopen(path.c_str(),"r");
  if(f == NULL) {
    *file = NULL;
    fprintf(stderr,"open file:%s failed",path.c_str());
    return false;
  } else {
    *file = new File(path,f);
  }
  return true;
}

File::File(const std::string& fname, FILE* f) :
    filename_(fname), file_(f) {}

bool File::Read(char **buffer) {
  fseek(file_, 0, SEEK_END);
  size_t fsize = ftell(file_);
  rewind(file_);
  *buffer = (char*)malloc(sizeof(char)*fsize);
  size_t r = fread(*buffer, 1, fsize, file_);
  if(r != fsize) {
    fprintf(stderr, "file:%s read error!", filename_.c_str());
    return false;
  }
  return true;
}

bool File::Append(const char *data,size_t n) {
  size_t r = fwrite(data, 1, n, file_);
  if(r != n) {
    fprintf(stderr, "file:%s write error!", filename_.c_str());
    return false;
  }
  return true;
}

bool File::Close() {
  if(fclose(file_) != 0) {
    fprintf(stderr, "file:%s close error!", filename_.c_str());
    return false;
  }
  return true;
}

bool File::Flush() {
  if(fflush(file_) != 0) {
    fprintf(stderr, "file:%s flush error!", filename_.c_str());
    return false;
  }
  return true;
}

std::string MD5(std::streambuf *file)
{
  char buf[64 * 1024];
  EVP_MD_CTX  md;
  EVP_MD_CTX_init( &md );
  EVP_DigestInit_ex( &md, EVP_md5(), 0 );
  std::size_t count = 0 ;
  while ( (count = file->sgetn( buf, sizeof(buf) )) > 0 ) {
    EVP_DigestUpdate( &md, buf, count );
  }
  unsigned int md5_size = EVP_MAX_MD_SIZE;
  unsigned char md5[EVP_MAX_MD_SIZE];
  EVP_DigestFinal_ex( &md, md5, &md5_size );
  std::ostringstream ss;
  for ( unsigned int i = 0 ; i < md5_size ; i++ )
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(md5[i]);
  return ss.str();
}

}  // namespace by
