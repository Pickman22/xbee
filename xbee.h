#ifndef XBEE_H
#define XBEE_H

#include <QSerialPort>
#include <string>
#include <vector>
#include <stdint.h>
#include <QThread>
#include <QDebug>
#include <string>

const u_int64_t broad_cast_address = 0xffff;
const u_int64_t broad_cast_network_address = 0xfffe;

const int wait_for_bytes_written = -1;

const size_t remote_at_cmd_rq_len = 20; // length of whole request commands.
const u_int8_t api_remote_at_cmd_rq_id = 0x17;
const u_int8_t api_remote_at_cmd_rq_len = 0x10; // length of the api message (size between length and checksum bytes).

typedef u_int8_t frame_flag_t;
typedef u_int8_t pin_function_t;
typedef u_int8_t pin_digital_output_t;
typedef u_int8_t remote_cmd_opt_t;

const remote_cmd_opt_t disable_retries = 0x01;
const remote_cmd_opt_t apply_changes = 0x02;
const remote_cmd_opt_t enable_aps_encryption = 0x20;
const remote_cmd_opt_t extended_tranmission_timeout = 0x40;

const frame_flag_t header_flag = 0x7e;
const frame_flag_t escape_flag = 0x7d;
const frame_flag_t xon_flag = 0x11;
const frame_flag_t xoff_flag = 0x14;

const u_int8_t to_char_offset = 48;

const pin_function_t AD0 = 0;
const pin_function_t AD1 = 1;
const pin_function_t AD2 = 2;
const pin_function_t AD3 = 3;
const pin_function_t AD4 = 4;
const pin_function_t AD5 = 5;

const pin_function_t DIO0 = 0;
const pin_function_t DIO1 = 1;
const pin_function_t DIO2 = 2;
const pin_function_t DIO3 = 3;
const pin_function_t DIO4 = 4;
const pin_function_t DIO5 = 5;
const pin_function_t DIO6 = 6;
const pin_function_t DIO7 = 7;
const pin_function_t DI8 = 8;

const pin_digital_output_t DOO = 0;
const pin_digital_output_t DO1 = 1;
const pin_digital_output_t DO2 = 2;
const pin_digital_output_t DO3 = 3;
const pin_digital_output_t DO4 = 4;
const pin_digital_output_t DO5 = 5;
const pin_digital_output_t DO6 = 6;
const pin_digital_output_t DO7 = 7;

typedef enum {
    adc = 2,
    digital_input = 3,
    digital_low = 4,
    digital_high = 5
} pin_config_t;

typedef enum {
    pin_0 = 0,
    pin_1,
    pin_2,
    pin_3,
    pin_4,
    pin_5,
    pin_6,
    pin_7
} pin_t;

typedef enum {
    pin_off = 4,
    pin_on = 5
} pin_state_t;

typedef struct {
    u_int8_t header;
    u_int8_t length[2];
    u_int8_t api_identifier;
    u_int8_t frame_id;
    u_int8_t address[8];
    u_int8_t network_address[2];
    u_int8_t cmd_options;
    u_int8_t cmd_name[2];
    u_int8_t cmd_data;
    u_int8_t checksum;
}remote_at_cmd_rq_t;

class Xbee : public QSerialPort
{
    Q_OBJECT

    int state, length;
    std::vector<u_int8_t> buffer;
    u_int8_t mcheck;

    u_int8_t calc_checksum(std::vector<u_int8_t> &data);

    void send_msg(remote_at_cmd_rq_t &msg);
    void parse_data(std::vector<u_int8_t>& data);
    void to_8bit_arr(u_int64_t add, u_int8_t *data);
    void to_8bit_arr(u_int16_t add, u_int8_t *data);
    void populate_arr(u_int64_t add, u_int8_t *data, size_t len);
    void pretty_print_message(remote_at_cmd_rq_t &msg);
    void set_checksum(remote_at_cmd_rq_t &msg);

public:
    explicit Xbee(QObject *parent = 0);
    explicit Xbee(QString port_name,
                  QSerialPort::BaudRate baud,
                  QObject *parent = 0,
                  QSerialPort::DataBits databits = QSerialPort::Data8,
                  QSerialPort::Direction dir = QSerialPort::AllDirections,
                  QIODevice::OpenMode mode = QSerialPort::ReadWrite,
                  QSerialPort::FlowControl flow = QSerialPort::NoFlowControl,
                  QSerialPort::Parity parity = QSerialPort::NoParity,
                  QSerialPort::StopBits stop = QSerialPort::OneStop);

    std::vector<u_int8_t> getData();
    void pinConfig(pin_t pin, pin_config_t config);
    void digitalOutput(pin_t pin, pin_state_t st);
    void compose_msg(std::vector<u_int8_t>& data, remote_at_cmd_rq_t& msg,
                     u_int8_t fid = 0x00,
                     u_int64_t add = broad_cast_address,
                     u_int16_t nwadd = broad_cast_network_address);

signals:
   void message_ready();

public slots:
    void on_ready_read();

};

#endif // XBEE_H
