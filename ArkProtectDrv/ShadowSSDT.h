#ifndef CXX_ShadowSSDT_H
#define CXX_ShadowSSDT_H

#include <ntifs.h>



BOOLEAN 
APGetKeServiceDescriptorTableShadow(OUT PUINT_PTR SSSDTAddress);


#endif // !CXX_ShadowSSDT_H

