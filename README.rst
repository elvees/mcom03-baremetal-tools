=========================
Baremetal-утилиты MCom-03
=========================

Репозиторий содержит baremetal-приложения для запуска на процессорах ARM CPU0 и MIPS32 RISC0.

.. contents:: Содержание

Тулчейны
========

Для сборки используются тулчейны ARM и RISC. Для RISC должен использоваться тулчейн производства
ЭЛВИС (тулчейн реализует обходы аппаратных ошибок ядра).

Оба тулчейна входят в состав `MCom-03 Linux SDK <https://dist.elvees.com/mcom03/docs/linux-sdk/>`_.
Описание тулчейнов — `Средства сборки <https://dist.elvees.com/mcom03/docs/linux-sdk/2023.03/components/buildroot.html#toolchain>`_

Подготовка среды::

  wget https://dist.elvees.com/mcom03/buildroot/2023.03/rockpi/images/aarch64-buildroot-linux-gnu_sdk-buildroot.tar.gz
  tar -xf aarch64-buildroot-linux-gnu_sdk-buildroot.tar.gz
  source aarch64-buildroot-linux-gnu_sdk-buildroot/environment-setup
  export MIPS32_CMAKE_TOOLCHAIN_FILE=$PWD/aarch64-buildroot-linux-gnu_sdk-buildroot/opt/toolchain-mipsel-elvees-elf32/share/cmake/toolchain.cmake
  export ARM_CMAKE_TOOLCHAIN_FILE=$PWD/aarch64-buildroot-linux-gnu_sdk-buildroot/share/buildroot/toolchainfile.cmake

.. note:: Ссылка на релиз MCom-03 Buidroot приведена справочно. Последняя версия данного репозитория
   может требовать более свежих версий компиляторов.

Сборка
======

Сборка проекта для MIPS32 (RISC0)::

  mkdir build-mips
  cmake -S . -B build-mips \
    -DCMAKE_TOOLCHAIN_FILE=$SDK/opt/toolchain-mipsel-elvees-elf32/share/cmake/toolchain.cmake
  make -j -C build-mips

Сборка проекта ARM64 (CPU0)::

  mkdir build-arm
  cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=$SDK/share/buildroot/toolchainfile.cmake
  make -j -C build-arm

Разработка
==========

Для проверки форматирования используется pre-commit (см. pre-commit.com)::

  pre-commit run --all-files

hello-world
===========

Приложение в бесконечном цикле выводит сообщение "Hello, world!" в UART0 и
переключает состояние GPIO1_PORTD_0 (на плате MCom-03 BuB к этому GPIO подключен
светодиод HL11).

Приложение собирается в двух (для MIPS) или трёх (для ARM64) вариантах:

* XIP для запуска с QSPI-памяти;
* RAM для запуска через JTAG или при загрузке через консоль UART;
* U-Boot для запуска из U-Boot (доступно только для ARM64).

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

Запуск через U-Boot
-------------------

#. Собрать приложение для ARM64.
#. Запустить U-Boot. При появлении соощения *Hit any key to stop autoboot* нажать любую клавишу,
   что бы перейти в режим командной строки.
#. Загрузить elf-файл в память любым способом. Например:

   * через UART0 по протоколу XMODEM::

       loadx $loadaddr

   * с SD-карты::

       load mmc 1:1 $loadaddr /spi-flasher-aarch64-uboot.elf

#. Запустить исполнение elf-файла::

     setenv autostart 1
     bootelf $loadaddr

.. note:: В U-Boot доступно только 256 МиБ DDR-памяти в диапазоне от 0x8_9040_0000 до 0x8_A03F_FFFF
   поэтому загружать elf-файл необходимо в пределах этого диапазона. Следует учитывать, что
   в верхней части этого диапазона располагается код U-Boot, который нельзя перекрывать.
   Адрес начала U-Boot можно посмотреть в поле `relocaddr`, выполнив команду `bdaddr`.
   Для избежания повреждения кода U-Boot при загрузке файла рекомендуется использовать
   адрес из переменной `loadaddr`. При выполнении команды `bootelf` elf-файл будет распакован
   по адресу 0x8_9100_0000.

spi-flasher
===========

Приложение предназначено для загрузки в режиме UART0 (BOOT=3) и предоставляет функционал для
работы с памятью SPI NOR, подключенной к QSPI0 или QSPI1. Приложение используется скриптом
`mcom03-flash-tools`__ для прошивки SPI Flash.

__ https://gerrit.elvees.com/gitweb?p=mcom03%2Fflash-tools.git;a=summary

Приложение spi-flasher предоставляет консоль через UART0 и может использоваться вручную без
mcom03-flash-tools (кроме записи данных, которые передаются в бинарном виде). Для ручного
запуска spi-flasher необходимо выполнить следующие действия:

#. Собрать spi-flasher для архитектуры MIPS (нужен файл spi-flasher-mips-ram.hex).
#. Выставить переключатели BOOT в состояние "011" и подать питание на MCom-03 (или нажать Reset,
   если питание уже было подано).
#. Выполнить::

    cat spi-flasher-mips-ram.hex > /dev/ttyUSB0

#. Открыть UART в любом текстовом терминале (например, ``minicom -D /dev/ttyUSB0``).
#. Выполнить команду ``run``. После этого начинает исполняться spi-flasher.

Основные команды:

* ``qspi <id> [v18]``
  ``<id>`` - выбор QSPI0 или QSPI1;
  ``[v18]`` - выбор напряжения КП QSPI1. Для QSPI0 значение ``[v18]`` игнорируется.
  ``[v18]`` = 0 - режим 3.3В, ``[v18]`` = 1 - режим 1.8В (например, ``qspi 1 0``).
* ``read <offset> <size> [text|bin]`` - чтение содержимого SPI Flash.
  ``<offset>`` - смещение, начиная с которого читать данные, ``<size>`` - размер данных.
  Если третий аргумент не указан или указан как ``text``, то данные выводятся в текстовом виде.
  Бинарный вид (``bin``) используется только для mcom03-flash-tools. Например, ``read 0 0x200``.
* ``erase <offset>`` - очистка сектора, начинающегося со смещения ``<offset>``. Размер сектора
  зависит от конкретной флеш-памяти (для S25FL128S сектор имеет размер 64 КиБ).
* ``custom <tx_data> <rx_size>`` - отправка на SPI Flash данных ``<tx_data>`` и вывод ``<rx_size>``
  байт ответа. ``<tx_data>`` - это набор байт, записанных слитно в 16-ричном представлении. Перед
  ``<tx_data>`` можно не указывать ``0x``. Например, команда ``custom 0b00020000 64`` или
  ``custom 0x0b00020000 64`` отправит на SPI 5 байт ``[0b, 00, 02, 00, 00]`` (команда FAST_READ,
  адрес 0x200 и один dummy-байт) и прочитает 64 байта ответа. ``<rx_size>`` может быть любым
  неотрицательным целым числом.
