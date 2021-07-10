Сборка
======

Для сборки выполнить:

#. Настроить среду сборки::

     export MODULEPATH+=:/usr/corp/Projects/ipcam-vip1/modules
     module load cmake

#. Выбрать один из тулчейнов

   * Для сборки под MIPS (RISC0)::

       module load toolchain/mips/codescape/img/bare/2018.09-03

   * Для сборки под ARM64 (CPU0)::

       module load toolchain/aarch64/bootlin/stable/2018.02

#. Запустить сборку::

     mkdir build && cd build
     cmake ../ -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE
     make

Разработка
==========

Для проверки форматирования используется pre-commit (см. pre-commit.com)::

  pre-commit run --all-files

hello-world
===========

Приложение в бесконечном цикле выводит сообщение "Hello, world!" в UART0 и
переключает состояние GPIO1_PORTD_0 (на плате MCom-03 BuB к этому GPIO подключен
светодиод HL11).

Приложение собирается в двух вариантах:

* XIP для запуска с QSPI-памяти;
* RAM для запуска через JTAG или при загрузке через консоль UART.

Запуск с QSPI0
--------------

#. Собрать приложение для MIPS32.
#. Прошить файл hello-world-mips-xip0.bin на SPI-флеш.
#. Подключить SPI-флеш в разъём QSPI0.
#. Выставить переключатели BOOT в состояние "000".
#. Подать питание на MCom-03.

Запуск с QSPI1
--------------

#. Собрать приложение для ARM64.
#. Прошить файл hello-world-aarch64-xip1.bin на SPI-флеш.
#. Подключить SPI-флеш в разъём QSPI1.
#. Выставить переключатели BOOT в состояние "101".
#. Подать питание на MCom-03.

Запуск через JTAG с RISC0
-------------------------

#. Собрать приложение для MIPS32.
#. Выставить переключатели BOOT в состояние "000" или "011".
#. Подать питание на MCom-03.
#. Запустить MDB и выполнить команды::

     loadelf <путь_к_файлу>/hello-world-mips-ram.elf
     set risc.pc 0xa0000000
     run

Запуск через JTAG с CPU0
-------------------------

#. Собрать приложение для ARM64.
#. Выставить переключатели BOOT в состояние "000" или "011".
#. Подать питание на MCom-03.
#. Запустить MDB и выполнить команды::

     set 0xbf000020 0x10
     loadbin <путь_к_файлу>/hello-world-aarch64-ram.bin 0xa0000000
     set 0xbf000000 0x10
     set 0xbf001008 0x115
     set 0xa1080000 0x2
     set 0xa1080004 0x2
     set 0xa1080008 0x2
     set 0xa1000040 0x10
     set 0xa100011C 0x0
     set 0xa1000000 0x10

Запуск через UART-консоль
-------------------------

#. Собрать приложение для MIPS32.
#. Выставить переключатели BOOT в состояние "011".
#. Подать питание на MCom-03.
#. Передать содержимое приложения в накристальное ОЗУ::

     cat hello-world-mips-ram.hex > /dev/ttyUSB0

#. Открыть UART-консоль::

     minicom -D /dev/ttyUSB0

#. В UART-консоли ввести команду::

     run
