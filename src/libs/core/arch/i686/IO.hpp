#include <core/Defs.h>
#include <stdint.h>

namespace arch {
namespace i686 {

    extern "C" void ASMCALL Outb(uint16_t port, uint8_t value);
    extern "C" uint8_t ASMCALL Inb(uint16_t port);

}
}