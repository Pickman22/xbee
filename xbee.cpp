#include "xbee.h"

Xbee::Xbee(QObject *parent) :
    QSerialPort(parent)
{
    this->setBaudRate(QSerialPort::Baud9600);
    this->setPortName("/dev/USB0");
    this->open(QIODevice::ReadWrite);
    state = 0;
    length = 0;
    mcheck = 0;
}

Xbee::Xbee(QString port_name,
           QSerialPort::BaudRate baud,
           QObject *parent,
           QSerialPort::DataBits data,
           QSerialPort::Direction dir,
           QIODevice::OpenMode mode,
           QSerialPort::FlowControl flow,
           QSerialPort::Parity parity,
           QSerialPort::StopBits stop) :
    QSerialPort(parent)
{
    this->setPortName(port_name);
    this->open(mode);
    this->setBaudRate(baud, dir);
    this->setDataBits(data);
    this->setFlowControl(flow);
    this->setParity(parity);
    this->setStopBits(stop);

    connect(this, SIGNAL(readyRead()), SLOT(on_ready_read()));
    state = 0;
    length = 0;
    mcheck = 0;
}

void Xbee::compose_msg(std::vector<u_int8_t> &data, remote_at_cmd_rq_t &msg, u_int8_t fid, u_int64_t add, u_int16_t nwadd) {
    msg.header = header_flag;
    msg.length[0] = 0x00;
    msg.length[1] = api_remote_at_cmd_rq_len; // Request messages length is always 0x10 = 16
    msg.api_identifier = api_remote_at_cmd_rq_id;
    msg.frame_id = fid;
    to_8bit_arr(add, &msg.address[0]);
    to_8bit_arr(nwadd, &msg.network_address[0]);
    msg.cmd_options = data[0];
    msg.cmd_name[0] = data[1];
    msg.cmd_name[1] = data[2];
    msg.cmd_data = data[3];
    set_checksum(msg);
    pretty_print_message(msg);
}

u_int8_t Xbee::calc_checksum(std::vector<u_int8_t> &data) {
    u_int8_t tmp = 0;
    for(std::vector<u_int8_t>::iterator it = data.begin(); it != data.end(); ++it) {
        tmp += *it;
    }
    return (0xff - tmp);
}

void Xbee::set_checksum(remote_at_cmd_rq_t &msg) {
    u_int8_t *ptr = &msg.api_identifier;
    u_int8_t tmp = 0;
    for(size_t i = 0; i < 16; ++i) {
        tmp += *ptr++;
    }
    msg.checksum = 0xff - tmp;
}

void Xbee::on_ready_read() {
    std::vector<u_int8_t> dat;
    if(bytesAvailable()) { // If there's data available to be read.
        QByteArray arr = readAll();
        dat = std::vector<u_int8_t>(arr.data(), arr.data() + arr.length());
    }
    parse_data(dat);
}

void Xbee::parse_data(std::vector<u_int8_t> &data) {
    for(std::vector<u_int8_t>::iterator it = data.begin(); it != data.end(); ++it) {
        switch(state) {
        case 0:
            if(*it == header_flag) {
                state++;
            } else {
                //qDebug() << "Waiting for start flag, got " << *it << " instead";
            }
            break;
        case 1:
            if(*it == 0x00) {
                state++;
            } else {
                state = 0;
                //qDebug() << "Error. This byte should be 0, got " << *it << " instead";
            }
            break;
        case 2:
            length = *it;
            state++;
            break;
        default:
            if(length--) {
                buffer.push_back(*it);
                mcheck += *it;
            } else if((mcheck + *it) == 0){
                emit message_ready();
                state = 0;
                length = 0;
                mcheck = 0;
                buffer.clear();
            }
        }
    }
}

void Xbee::digitalOutput(pin_t pin, pin_state_t st) {
    remote_at_cmd_rq_t msg;
    std::vector<u_int8_t> data(4, 0);
    data[0] = (u_int8_t)apply_changes;
    data[1] = (u_int8_t)'D';
    data[2] = (u_int8_t)(pin + to_char_offset);
    data[3] = (u_int8_t)st;
    compose_msg(data, msg);
    send_msg(msg);
}

void Xbee::send_msg(remote_at_cmd_rq_t &cmd) {
    char *msg = (char*)&cmd.header;
    write(msg, remote_at_cmd_rq_len);
    waitForBytesWritten(wait_for_bytes_written);
}

void Xbee::pinConfig(pin_t pin, pin_config_t config) {
    remote_at_cmd_rq_t msg;
    std::vector<u_int8_t> data(4, 0);
    data[0] = (u_int8_t)apply_changes;
    data[1] = (u_int8_t)'D';
    data[2] = (u_int8_t)(pin + to_char_offset);
    data[3] = (u_int8_t)config;
    compose_msg(data, msg);
    send_msg(msg);
}

void Xbee::to_8bit_arr(u_int16_t add, u_int8_t *data) {
    populate_arr((u_int64_t)add, data, sizeof add);
}

void Xbee::to_8bit_arr(u_int64_t add, u_int8_t *data) {
    populate_arr(add, data, sizeof add);
}

void Xbee::populate_arr(u_int64_t add, u_int8_t *data, size_t len) {
    // The MSB should be written to data[0] in order to be the first byte written to the serial port.
    for(size_t i = 0; i < len; ++i) {
        data[len - 1 - i] = (u_int8_t)((add >> (8 * i)) & 0x000000ff);
    }
}

void Xbee::pretty_print_message(remote_at_cmd_rq_t &msg) {
    qDebug() << "Header: " << hex << msg.header;
    qDebug() << "Length MSB: " << hex << msg.length[1];
    qDebug() << "Length LSB: " << hex << msg.length[0];
    qDebug() << "Frame type: " << hex << msg.api_identifier;
    qDebug() << "Frame id: " << hex << msg.frame_id;
    for(int i = 0; i < 8; ++i) {
        qDebug() << "Address[" << i << "]: " << hex << msg.address[i];
    }
    for(int i = 0; i < 2; ++i) {
        qDebug() << "Network Address[" << i <<"]: " << hex << msg.network_address[i];
    }
    qDebug() << "Remote cmd opt: " << hex << msg.cmd_options;
    qDebug() << "AT cmd: " << hex << (char)msg.cmd_name[0] << (char)msg.cmd_name[1];
    qDebug() << "Cmd parameter: " << hex << msg.cmd_data;
    qDebug() << "Checksum: " <<  hex << msg.checksum;
}
