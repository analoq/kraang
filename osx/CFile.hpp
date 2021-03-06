#include <stdio.h>
#include "../File.hpp"

class CFile : public KFile
{
private:
  FILE *fp;
  bool is_valid;
public:
  CFile(const char *path)
  {
    fp = fopen(path, "rb");
    is_valid = (fp != NULL);
  }

  bool isValid()
  {
    return is_valid;
  }

  void close()
  {
    fclose(fp);
  }

  uint8_t readByte()
  {
    return fgetc(fp);
  }

  void read(uint32_t length, uint8_t *data)
  {
    fread(data, length, 1, fp);
  }

  uint32_t getPosition()
  {
    return ftell(fp);
  }

  void seek(int32_t position)
  {
    fseek(fp, position, SEEK_CUR);
  }
};
