#ifndef FILE_HPP
#define FILE_HPP
class KFile
{
public:
  virtual bool isValid() = 0;

  virtual void close() = 0;

  virtual uint8_t readByte() = 0;

  virtual void read(uint32_t length, uint8_t *data) = 0;

  virtual uint32_t getPosition() = 0;

  virtual void seek(int32_t position) = 0;
};
#endif
