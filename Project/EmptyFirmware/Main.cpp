
// Copyright (C) 2012 R. Diez
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the Affero GNU General Public License version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero GNU General Public License version 3 for more details.
//
// You should have received a copy of the Affero GNU General Public License version 3
// along with this program. If not, see http://www.gnu.org/licenses/ .


#include <assert.h>

#include <BareMetalSupport/AssertionUtils.h>
#include <BareMetalSupport/Miscellaneous.h>
#include <BareMetalSupport/BoardInitUtils.h>
#include <BareMetalSupport/SerialPortUtils.h>

#include <ArduinoDueUtils/ArduinoDueUtils.h>

#include <sam3xa.h>  // All interrupt handlers must probably be extern "C", so include their declarations here.

#include <pio.h>


#define EOL "\r\n"  // Carriage Return, 0x0D, followed by a Line Feed, 0x0A.


static const bool ENABLE_DEBUG_CONSOLE = true;


static void PrintPanicMsg ( const char * const msg )
{
  if ( ENABLE_DEBUG_CONSOLE )
  {
    // This routine is called with interrupts disabled and should rely
    // on as little other code as possible.
    SerialSyncWriteStr( EOL );
    SerialSyncWriteStr( "PANIC: " );
    SerialSyncWriteStr( msg );
    SerialSyncWriteStr( EOL );

    // Here it would be a good place to print a stack backtrace,
    // but I have not been able to figure out yet how to do that
    // with the ARM Thumb platform.
  }
}


static void Configure ( void )
{
  // ------- Configure the UART connected to the AVR controller -------

  if ( ENABLE_DEBUG_CONSOLE )
  {
    VERIFY( pio_configure( PIOA, PIO_PERIPH_A,
                           PIO_PA8A_URXD | PIO_PA9A_UTXD, PIO_DEFAULT ) );

    // Enable the pull-up resistor for RX0.
    pio_pull_up( PIOA, PIO_PA8A_URXD, ENABLE ) ;

    InitSerialPort( false );

    SerialSyncWriteStr( "--- EmptyDue " PACKAGE_VERSION " ---" EOL );
    SerialSyncWriteStr( "Welcome to the Arduino Due's programming USB serial port." EOL );
  }

  SetUserPanicMsgFunction( &PrintPanicMsg );


  // ------- Perform some assorted checks -------

  assert( IsJtagTdoPullUpActive() );

  // Check that the brown-out detector is active.
  #ifndef NDEBUG
    const uint32_t supcMr = SUPC->SUPC_MR;
    assert( ( supcMr & SUPC_MR_BODDIS   ) == SUPC_MR_BODDIS_ENABLE   );
    assert( ( supcMr & SUPC_MR_BODRSTEN ) == SUPC_MR_BODRSTEN_ENABLE );
  #endif


  // ------- Configure the watchdog -------

  WDT->WDT_MR = WDT_MR_WDDIS;
}


// These symbols are defined in the linker script file.
extern "C" int _sfixed;
extern "C" int _etext;
extern "C" int _sbss;
extern "C" int _ebss;
extern "C" int _srelocate;
extern "C" int _erelocate;


void StartOfUserCode ( void )
{
    Configure();

    if ( ENABLE_DEBUG_CONSOLE )
    {
      const unsigned codeSize     = unsigned( uintptr_t( &_etext     ) - uintptr_t( &_sfixed    ) );
      const unsigned initDataSize = unsigned( uintptr_t( &_erelocate ) - uintptr_t( &_srelocate ) );
      const unsigned bssDataSize  = unsigned( uintptr_t( &_ebss      ) - uintptr_t( &_sbss      ) );

      SerialSyncWriteStr( "Code size: 0x" );
      SerialSyncWriteUint32Hex( codeSize );
      SerialSyncWriteStr( ", initialised data size: 0x" );
      SerialSyncWriteUint32Hex( initDataSize );
      SerialSyncWriteStr( ", BSS size: 0x" );
      SerialSyncWriteUint32Hex( bssDataSize );
      SerialSyncWriteStr( "." EOL );
    }


    // ------ Main loop ------

    if ( ENABLE_DEBUG_CONSOLE )
    {
      SerialSyncWriteStr( "Entering the main loop, which just waits forever." EOL );
    }

    for (;;)
    {
    }
}


void HardFault_Handler ( void )
{
  // Note that instruction BKPT causes a HardFault when no debugger is currently attached.

  if ( ENABLE_DEBUG_CONSOLE )
  {
    SerialSyncWriteStr( "HardFault" EOL );
  }

  ForeverHangAfterPanic();
}
