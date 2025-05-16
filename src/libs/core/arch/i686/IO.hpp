#include <Defs.hpp>
#include <stdint.h>

namespace arch {
namespace i686 {

    EXPORT void ASMCALL Outb(uint16_t port, uint8_t value);
    EXPORT uint8_t ASMCALL Inb(uint16_t port);

}
}