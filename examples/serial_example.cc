#include <iostream>

#include "serial/serial.h"

// /***
//  * This example expects the serial port has a loopback on it.
//  *
//  * Alternatively, you could use an Arduino:
//  *
//  * <pre>
//  *  void setup() {
//  *    Serial.begin(<insert your baudrate here>);
//  *  }
//  *
//  *  void loop() {
//  *    if (Serial.available()) {
//  *      Serial.write(Serial.read());
//  *    }
//  *  }
//  * </pre>
//  */

// #include <string>
// #include <iostream>
// #include <cstdio>

// // OS Specific sleep
// #ifdef _WIN32
// #include <windows.h>
// #else
// #include <unistd.h>
// #endif

// #include "serial/serial.h"

// using std::string;
// using std::exception;
// using std::cout;
// using std::cerr;
// using std::endl;
// using std::vector;

// void my_sleep(unsigned long milliseconds) {
// #ifdef _WIN32
//     Sleep(milliseconds); // 100 ms
// #else
//     usleep(milliseconds*1000); // 100 ms
// #endif
// }

static void enumerate_ports()
{
    std::vector<serial::PortInfo> devices_found = serial::list_ports();

    for (const auto& info : devices_found) {
        printf( "(%s, %s, %s)\n",  info.port.c_str(), info.description.c_str(), info.hardware_id.c_str() );
    }
}

// void print_usage()
// {
//     cerr << "Usage: test_serial {-e|<serial port address>} ";
//     cerr << "<baudrate> [test string]" << endl;
// }

// int run(int argc, char **argv)
// {
//     if(argc < 2) {
//         print_usage();
//         return 0;
//     }

//     // Argument 1 is the serial port or enumerate flag
//     string port(argv[1]);

//     if( port == "-e" ) {
//         enumerate_ports();
//         return 0;
//     }
//     else if( argc < 3 ) {
//         print_usage();
//         return 1;
//     }

//     // Argument 2 is the baudrate
//     unsigned long baud = 0;
// #if defined(WIN32) && !defined(__MINGW32__)
//     sscanf_s(argv[2], "%lu", &baud);
// #else
//     sscanf(argv[2], "%lu", &baud);
// #endif

//     // port, baudrate, timeout in milliseconds
//     serial::Serial my_serial(port, baud, serial::Timeout::simpleTimeout(1000));

//     cout << "Is the serial port open?";
//     if(my_serial.isOpen())
//         cout << " Yes." << endl;
//     else
//         cout << " No." << endl;

//     // Get the Test string
//     int count = 0;
//     string test_string;
//     if (argc == 4) {
//         test_string = argv[3];
//     } else {
//         test_string = "Testing.";
//     }

//     // Test the timeout, there should be 1 second between prints
//     cout << "Timeout == 1000ms, asking for 1 more byte than written." << endl;
//     while (count < 10) {
//         size_t bytes_wrote = my_serial.write(test_string);

//         string result = my_serial.read(test_string.length()+1);

//         cout << "Iteration: " << count << ", Bytes written: ";
//         cout << bytes_wrote << ", Bytes read: ";
//         cout << result.length() << ", String read: " << result << endl;

//         count += 1;
//     }

//     // Test the timeout at 250ms
//     my_serial.setTimeout(serial::Timeout::max(), 250, 0, 250, 0);
//     count = 0;
//     cout << "Timeout == 250ms, asking for 1 more byte than written." << endl;
//     while (count < 10) {
//         size_t bytes_wrote = my_serial.write(test_string);

//         string result = my_serial.read(test_string.length()+1);

//         cout << "Iteration: " << count << ", Bytes written: ";
//         cout << bytes_wrote << ", Bytes read: ";
//         cout << result.length() << ", String read: " << result << endl;

//         count += 1;
//     }

//     // Test the timeout at 250ms, but asking exactly for what was written
//     count = 0;
//     cout << "Timeout == 250ms, asking for exactly what was written." << endl;
//     while (count < 10) {
//         size_t bytes_wrote = my_serial.write(test_string);

//         string result = my_serial.read(test_string.length());

//         cout << "Iteration: " << count << ", Bytes written: ";
//         cout << bytes_wrote << ", Bytes read: ";
//         cout << result.length() << ", String read: " << result << endl;

//         count += 1;
//     }

//     // Test the timeout at 250ms, but asking for 1 less than what was written
//     count = 0;
//     cout << "Timeout == 250ms, asking for 1 less than was written." << endl;
//     while (count < 10) {
//         size_t bytes_wrote = my_serial.write(test_string);

//         string result = my_serial.read(test_string.length()-1);

//         cout << "Iteration: " << count << ", Bytes written: ";
//         cout << bytes_wrote << ", Bytes read: ";
//         cout << result.length() << ", String read: " << result << endl;

//         count += 1;
//     }

//     return 0;
// }


int main(int argc, char** argv)
{
    enumerate_ports();

    {
        serial::Serial s;
        std::cout << std::boolalpha << s.isOpen() << "\n";
        std::cout << "Port: " << s.getPort() << "\n";
    }

    {
        try {
            serial::Serial s1("port1");
        }
        catch (const std::exception& e) {
            std::cerr << "Unhandled Exception: " << e.what() << std::endl;
        }
    }

    {
        try {
            serial::Serial s1;

            size_t n_available = s1.available();
            s1.setTimeout(serial::Timeout());
            s1.getTimeout();

            s1.waitByteTimes(0);
            s1.waitReadable();

            s1.setBaudrate(9600);
            s1.getBaudrate();

            s1.setBytesize(serial::bytesize_t::fivebits);
            s1.getBytesize();

            s1.setFlowcontrol(serial::flowcontrol_t::flowcontrol_hardware);
            s1.getFlowcontrol();

            s1.setParity(serial::parity_t::parity_even);
            s1.getParity();

            s1.setStopbits(serial::stopbits_t::stopbits_two);
            s1.getStopbits();

            s1.waitForChange();

            s1.getCTS();

            s1.getDSR();

            s1.getRI();

            s1.getCD();

            s1.sendBreak(0);
            s1.setBreak(true);
            s1.setRTS(true);
            s1.setDTR(true);

            s1.flush();
            s1.flushInput();
            s1.flushOutput();

            s1.setPort("port2");

            s1.write(std::string("Hello, world"));
            s1.write(std::vector<uint8_t>{0x00, 0x01, 0x02});
        }
        catch (const std::exception& e) {
            std::cerr << "Unhandled Exception: " << e.what() << std::endl;
        }
    }

    return 0;
}
