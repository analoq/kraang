#include <thread>

using namespace std;

class CTiming
{
private:
  chrono::time_point<std::chrono::high_resolution_clock> start;
public:
  CTiming()
  {
    start = chrono::high_resolution_clock::now();
  }

  uint32_t getMicroseconds() const
  {
    auto now = chrono::high_resolution_clock::now();
    double diff = chrono::duration<double, std::micro>(now - start).count();
    return static_cast<uint32_t>(diff);
  }
 
  void delay(uint32_t us)
  {
    this_thread::sleep_for(chrono::microseconds{us});
  }
};

