#include "rigctl.h"
#include <math.h>
#include <map>

namespace net::rigctl {
    int recvLine(std::shared_ptr<net::Socket> sock, std::vector<std::string>& args);

    std::map<Mode, std::string> radioModeToString = {
        { MODE_USB, "USB" },
        { MODE_LSB, "LSB" },
        { MODE_CW, "CW" },
        { MODE_CWR, "CWR" },
        { MODE_RTTY, "RTTY" },
        { MODE_RTTYR, "RTTYR" },
        { MODE_AM, "AM" },
        { MODE_FM, "FM" },
        { MODE_WFM, "WFM" },
        { MODE_AMS, "AMS" },
        { MODE_PKTLSB, "PKTLSB" },
        { MODE_PKTUSB, "PKTUSB" },
        { MODE_PKTFM, "PKTFM" },
        { MODE_ECSSUSB, "ECSSUSB" },
        { MODE_ECSSLSB, "ECSSLSB" },
        { MODE_FA, "FA" },
        { MODE_SAM, "SAM" },
        { MODE_SAL, "SAL" },
        { MODE_SAH, "SAH" },
        { MODE_DSB, "DSB" }
    };

    // options taken from Hamlib src/misc.c
    // notice there are some duplicate values, but keys are unique
    // there's lots of options in Hamlib not in net::rigctl::Modes
    // there's one option in net::rigctl::Modes not in Hamlib
    std::map<std::string, Mode> radioStringToMode = {
        { "USB", MODE_USB },
        { "LSB", MODE_LSB },
        { "CW", MODE_CW },
        { "CWR", MODE_CWR },
        { "CW-R", MODE_CWR },
        { "RTTY", MODE_RTTY },
        { "RTTYR", MODE_RTTYR },
        { "RTTY-R", MODE_RTTYR },
        { "AM", MODE_AM },
        { "FM", MODE_FM },
        { "WFM", MODE_WFM },
        { "AMS", MODE_AMS },
        { "PKTLSB", MODE_PKTLSB },
        { "LSB-D", MODE_PKTLSB },
        { "PKTUSB", MODE_PKTUSB },
        { "USB-D", MODE_PKTUSB },
        { "PKTFM", MODE_PKTFM },
        { "FM-D", MODE_PKTFM },
        { "ECSSUSB", MODE_ECSSUSB },
        { "ECSSLSB", MODE_ECSSLSB },
        //{ "???", MODE_FA }, // not in Hamlib
        { "SAM", MODE_SAM },
        { "SAL", MODE_SAL },
        { "SAH", MODE_SAH },
        { "DSB", MODE_DSB }
    };

    Client::Client(std::shared_ptr<Socket> sock) {
        this->sock = sock;
    }

    Client::~Client() {
        close();
    }

    bool Client::isOpen() {
        return sock->isOpen();
    }

    void Client::close() {
        sock->close();
    }

    double Client::getFreq() {
        return getFloat("f");
    }

    int Client::setFreq(double freq) {
        return setFloat("F", freq);
    }

    Mode Client::getMode(int* passband) {
        sock->sendstr("m\n");
        std::vector<std::string> args;
        int err;

        //get the first line which is mode
        err = recvLine(sock, args);
        if (err != 1) {
            return MODE_INVALID;
        }
        auto it = radioStringToMode.find(args[0]);
        if (radioStringToMode.end() == it) {
            // consume passband to try and leave the client in a
            // good state
            recvLine(sock, args);
            return MODE_INVALID;
        }
        Mode mode = it->second;
        args.clear();

        //get the second line which is passband
        err = recvLine(sock, args);
        if (err != 1) {
            return MODE_INVALID;
        }
        *passband = std::stoi(args[0]);

        return mode;
    }

    int Client::setMode(Mode mode, int passband) {
        if (MODE_INVALID == mode) {
            return -1;
        }
        char buf[128];
        snprintf(buf, sizeof(buf), "%s %s %d\n", "M", radioModeToString[mode].c_str(), passband);
        sock->sendstr(buf);

        return recvStatus();
    }

    // TODO: get/setVFO

    double Client::getRIT() {
        return getFloat("j");
    }

    int Client::setRIT(double rit) {
        return setFloat("J", rit);
    }

    double Client::getXIT() {
        return getFloat("z");
    }

    int Client::setXIT(double rit) {
        return setFloat("Z", rit);
    }

    int Client::getPTT() {
        return getInt("t");
    }

    int Client::setPTT(bool ptt) {
        return setInt("T", ptt);
    }

    // TODO: get/setSplitVFO

    double Client::getSplitFreq() {
        return getFloat("i");
    }

    int Client::setSplitFreq(double splitFreq) {
        return setFloat("I", splitFreq);
    }

    // TODO: get/setSplitMode

    int Client::getAntenna() {
        return getInt("y");
    }

    int Client::setAntenna(int ant) {
        return setInt("Y", ant);
    }

    // TODO: sendMorse

    int Client::getDCD() {
        return getInt("\x8B");
    }

    // TODO: get/setRepeaterShift

    double Client::getRepeaterOffset() {
        return getFloat("o");
    }

    int Client::setRepeaterOffset(double offset) {
        return setFloat("O", offset);
    }

    double Client::getCTCSSTone() {
        return (double)getInt("c") / 10.0;
    }

    int Client::setCTCSSTone(double tone) {
        return setInt("C", (int)round(tone * 10.0));
    }

    // TODO: get/setDCSCode

    double Client::getCTCSSSquelch() {
        return getFloat("\x91");
    }

    int Client::setCTCSSSquelch(double squelch) {
        return setFloat("\x90", squelch);
    }

    // TODO: get/setDCSSquelch

    double Client::getTuningStep() {
        return getFloat("n");
    }

    int Client::setTuningStep(double step) {
        return setFloat("N", step);
    }

    // TODO: A bunch of other shit

    int Client::setBank(int bank) {
        return setInt("B", bank);
    }

    int Client::getMem() {
        return getInt("e");
    }

    int Client::setMem(int mem) {
        return setInt("E", mem);
    }

    int recvLine(std::shared_ptr<net::Socket> sock, std::vector<std::string>& args) {
        // Read line
        std::string line = "";
        int err = sock->recvline(line, 0, 1000);
        if (err <= 0) { return err; }

        // Split
        for (int i = 0; i < line.size(); i++) {
            // Skip spaces
            for (; line[i] == ' '; i++);
            if (i == line.size()) { break; }

            // Read part
            std::string part = "";
            for (; i < line.size() && line[i] != ' '; i++) { part += line[i]; }
            args.push_back(part);
        }

        return args.size();
    }

    int Client::recvStatus() {
        // Read line
        std::vector<std::string> args;
        int err = recvLine(sock, args);
        if (err != 2) { return -1; }
        
        // Check format
        if (args[0] != "RPRT") { return -1; }

        // Decode status
        return std::stoi(args[1]);
    }

    int Client::getInt(std::string cmd) {
        // Send command
        sock->sendstr(cmd + "\n");

        // Read line
        std::vector<std::string> args;
        int err = recvLine(sock, args);
        if (err != 1) { return -1; }

        // Decode frequency
        return std::stoi(args[0]); 
    }
    
    int Client::setInt(std::string cmd, int value) {
        // Send command
        char buf[128];
        sprintf(buf, "%s %d\n", cmd.c_str(), value);
        sock->sendstr(buf);

        // Receive status
        return recvStatus();
    }

    double Client::getFloat(std::string cmd) {
        // Send command
        sock->sendstr(cmd + "\n");

        // Read line
        std::vector<std::string> args;
        int err = recvLine(sock, args);
        if (err != 1) { return -1; }

        // Decode frequency
        return std::stod(args[0]);
    }

    int Client::setFloat(std::string cmd, double value) {
        // Send command
        char buf[128];
        sprintf(buf, "%s %lf\n", cmd.c_str(), value);
        sock->sendstr(buf);

        // Receive status
        return recvStatus();
    }

    std::string Client::getString(std::string cmd) {
        // TODO
        return "";
    }

    int Client::setString(std::string cmd, std::string value) {
        // TODO
        return -1;
    }

    std::shared_ptr<Client> connect(std::string host, int port) {
        return std::make_shared<Client>(net::connect(host, port));
    }

    Server::Server(std::shared_ptr<net::Listener> listener) {
        this->listener = listener;
        listenThread = std::thread(&Server::listenWorker, this);
    }

    Server::~Server() {
        stop();
    }

    bool Server::listening() {
        return listener->listening();
    }

    void Server::stop() {
        // Stop listen worker
        listener->stop();
        if (listenThread.joinable()) { listenThread.join(); }

        // Stop individual client threads
        while (true) {
            // Check how many sockets are left and stop if they're all closed
            
        }
    }

    void Server::listenWorker() {
        while (true) {
            // Wait for new client
            auto sock = listener->accept();
            if (!sock) { break; }

            // Add socket to list
            {
                std::lock_guard<std::mutex> lck(socketsMtx);
                sockets.push_back(sock);
            }

            // Start handler thread
            std::thread acceptThread(&Server::acceptWorker, this, sock);
            acceptThread.detach();
        }
    }

    void Server::acceptWorker(std::shared_ptr<Socket> sock) {
        while (true) {
            // Receive command
            std::vector<std::string> args;
            int argCount = recvLine(sock, args);
            if (argCount <= 0) { break; }

            // Parse command
            std::string cmd = args[0];
            if (cmd == "f" || cmd == "\\get_freq") {
                // TODO: Check if handler exists
                double freq = 0;
                int res = getFreqHandler(freq);
                if (!res) {
                    sendFloat(sock, freq);
                }
                else {
                    sendStatus(sock, res);
                }
            }
            if (cmd == "F" || cmd == "\\set_freq") {
                
            }
            else {
                // Return error
                sendStatus(sock, -1);
            }
        }
    }

    void Server::sendStatus(std::shared_ptr<Socket> sock, int status) {
        char buf[128];
        sprintf(buf, "RPRT %d\n", status);
        sock->sendstr(buf);
    }

    void Server::sendInt(std::shared_ptr<Socket> sock, int value) {
        char buf[128];
        sprintf(buf, "%d\n", value);
        sock->sendstr(buf);
    }

    void Server::sendFloat(std::shared_ptr<Socket> sock, double value) {
        char buf[128];
        sprintf(buf, "%lf\n", value);
        sock->sendstr(buf);
    }
}
