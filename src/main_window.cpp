/**
 * @file /src/main_window.cpp
 *
 * @brief Implementation for the qt gui->
 *
 * @date August 2024
 **/
/*****************************************************************************
** Includes
*****************************************************************************/

#include "../include/gamecontroller/main_window.hpp"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::RobocupController)
{
  ui->setupUi(this);

  bool isInit = initAddrAndPort();
      if(isInit){
        std::cout << "===== GAME CONTROLLER OPEN =====" << std::endl;
    } else {
        std::cout << "======= INITIALIZE FAIL ========" << std::endl;
        exit(0);
    }

  QIcon icon("://ros-icon.png");
  this->setWindowIcon(icon);

  qnode = new QNode();

  QObject::connect(qnode, SIGNAL(rosShutDown()), this, SLOT(close()));
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  QMainWindow::closeEvent(event);
}

MainWindow::~MainWindow()
{
  delete ui;
  delete m_pReadSocket;
  delete m_pSendSocket;
}

bool MainWindow::initAddrAndPort()
{
    bool isOpen_Network = false;
    bool isTimeOut = false;

    clock_t timeoutCnt_start = clock();
    clock_t timeoutCnt_end = clock();

    while(!isOpen_Network)
    {
        timeoutCnt_end = clock();
        if(double(timeoutCnt_end - timeoutCnt_start)/CLOCKS_PER_SEC > 1){
            isTimeOut = true;
        }

        QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();

        for(int i=0;i<ipAddressesList.size();i++)
        {
//            std::cout << i << " " << ipAddressesList.at(i).toString().toStdString() << std::endl;
            if(ipAddressesList.at(i) != QHostAddress::LocalHost && ipAddressesList.at(i).toIPv4Address())
            {
                if(isTimeOut){
                    if(ipAddressesList.at(i).toString().toStdString().find("172") != std::string::npos) // 192
                    {
                        m_qstrIp = ipAddressesList.at(i).toString();
                        isOpen_Network = true;
                        break;
                    }
                } else {
                    if(ipAddressesList.at(i).toString().toStdString().find("192") != std::string::npos) // 192
                    {
                        m_qstrIp = ipAddressesList.at(i).toString();
                        isOpen_Network = true;
                        break;
                    }
                }
            }
        }

        if(m_qstrIp.isEmpty())
        {
            m_qstrIp = QHostAddress(QHostAddress::LocalHost).toString();
        }
    }

    m_iPort = 3838;

    return isOpen_Network;
}

void MainWindow::initSocket()
{
    cout << "initSocket" << endl;
    if(m_pReadSocket==nullptr)
    {
        m_pReadSocket = new QUdpSocket(this);
    }

    if(m_pReadSocket!=nullptr && m_pReadSocket->bind(m_iPort, QUdpSocket::ShareAddress))
    {
        m_bIsServerOpen=true;
        ui->checkBox_side->setEnabled(false);
        ui->checkBox_kickoff->setEnabled(false);
        ui->comboBox_number->setEnabled(false);
        ui->comboBox_position->setEnabled(false);
        ui->comboBox_team->setEnabled(false);

        ui->Server_Open->setText("CLOSE");

        connect(m_pReadSocket,SIGNAL(readyRead()),this,SLOT(readData()));
        readData();
    }
}

void MainWindow::closeSocket()
{
    ui->checkBox_side->setEnabled(true);
    ui->checkBox_kickoff->setEnabled(true);
    ui->comboBox_number->setEnabled(true);
    if(ui->comboBox_position->currentIndex() != POSITION_TECHNICAL)
    {
        ui->comboBox_position->setEnabled(true);
    }
    ui->comboBox_state->setEnabled(true);
    ui->comboBox_team->setEnabled(true);
    ui->checkBox_side->setTristate(false);
    ui->checkBox_side->setText("Left (first half)");

    disconnect(m_pReadSocket,SIGNAL(readyRead()),this,SLOT(readData()));
    ui->textEdit->clear();

    ui->Server_Open->setText("OPEN");

    m_pReadSocket->close();

    m_bIsServerOpen = false;
}

void MainWindow::on_comboBox_position_currentIndexChanged(int index)
{
    if(index == POSITION_TECHNICAL)
    {
        ChangeTechnicalMode();
        ui->Server_Open->setEnabled(false);

        return;
    }

    if(index != POSITION){
        ui->Server_Open->setEnabled(true);
    } else {
        ui->Server_Open->setEnabled(false);
    }
}

void MainWindow::on_comboBox_state_currentIndexChanged(int index)
{
    qnode->gameControlData.robotnum = playerNum + 1;
    qnode->gameControlData.myside = initSide;
    qnode->gameControlData.position = position;
    qnode->gameControlData.state = index;

    if(index == 1) //state: ready
    {
        qnode->gameControlData.readytime = 30;
    }
    else
    {
        qnode->gameControlData.readytime = 0;
    }

    qnode->gamecontrollerPub->publish(qnode->gameControlData);
}

void MainWindow::on_Server_Open_clicked()
{
    if(m_bIsServerOpen == false)
    {
        playerNum = ui->comboBox_number->currentIndex();

        initSide = ui->checkBox_side->isChecked() ? LEFT : RIGHT;

        position = ui->comboBox_position->currentIndex();

        switch (ui->comboBox_team->currentIndex()) {
        case 0: // ROBIT
          myTeam = TEAM_ROBIT;
          break;
        case 1: //ROBIT_RED
          myTeam = TEAM_ROBIT_RED;
          break;
        case 2: //ROBIT_BLUE
          myTeam = TEAM_ROBIT_BLUE;
          break;
        default:
          myTeam = TEAM_ROBIT;
          ui->comboBox_team->setCurrentIndex(0);
          break;
        }
        qnode->gameControlData.myteam = myTeam;

        initSocket();

        qnode->gameControlData.robotnum = playerNum + 1;
        qnode->gameControlData.myside = initSide;
        qnode->gameControlData.iskickoff = ui->checkBox_kickoff->isChecked() ? true : false;
        qnode->gameControlData.position = position;
        qnode->gameControlData.state = ui->comboBox_state->currentIndex();
        qnode->gamecontrollerPub->publish(qnode->gameControlData);

        g_timer = new QTimer(this);
        QObject::connect(g_timer,SIGNAL(timeout()),this,SLOT(udpSend_callback()));
        g_timer->start(100);
    }
    else
    {
        closeSocket();
        delete g_timer;
    }

}

void MainWindow::ChangeTechnicalMode()
{
    cout << "!!TECHNICAL MODE ON!!" << endl;

    ui->comboBox_position->setEnabled(false);

    ui->label_title->setText("TECHNICAL CONTROLLER");
    ui->label_title->setStyleSheet("font-size:20pt");

    for(int i = 0; i < 5; i++)
    {
        ui->comboBox_state->removeItem(0);
    }

    ui->comboBox_state->addItem("TECHNICAL");
    ui->comboBox_state->addItem("LOCAL KICKER");
    ui->comboBox_state->addItem("LOCAL HELPER");
    ui->comboBox_state->addItem("DYNAMIC KICK");
    ui->comboBox_state->addItem("HIGH KICK");
    ui->comboBox_state->addItem("PARKOUR");
    ui->comboBox_state->addItem("OBSTACLE");

    playerNum = ui->comboBox_number->currentIndex();
    position = ui->comboBox_position->currentIndex();

    ui->comboBox_number->setEnabled(false);
    ui->comboBox_team->setEnabled(false);
    ui->checkBox_side->setEnabled(false);
    ui->checkBox_kickoff->setEnabled(false);

    ui->textEdit->deleteLater();
}

void MainWindow::readData()
{
    QByteArray str;
    str.resize(m_pReadSocket->bytesAvailable());

    m_pReadSocket->readDatagram(str.data(),str.size(),&senderAddress,&senderPort);

    qDebug() << "From : "<<senderAddress.toString();
    qDebug() << "Port : "<<senderPort;
    qDebug() << "Message : "<<str.size();

    cout<<"============= GAME INFO ============="<<endl;
    cout<<"SIDE : "<<mySide<<endl;
    cout<<"playerNum = "<<playerNum + 1<<endl;
    cout<<"position  = "<<position<<endl;

    if(str.size() == 688)
    {
        ui->textEdit->clear();

        memmove(&robocupData, str.data(), sizeof(RoboCupGameControlData));
        cout << "state = " << (int)robocupData.state << endl;
        cout << "penalty = " << (int)robocupData.teams[mySide].players[playerNum].penalty << endl;
        cout << "half = " << (int)robocupData.firstHalf << endl;
        cout << "kickoff team = " << (int)robocupData.kickOffTeam << endl;
        cout << "secondary = " << (int)robocupData.secondaryState << endl;
        cout<<"Team 0 Number"<<(int)robocupData.teams[0].teamNumber<<endl;
        cout<<"Team 1 Number"<<(int)robocupData.teams[1].teamNumber<<endl;

        mySide = robocupData.teams[0].teamNumber == myTeam ? 0 : 1;

        if(robocupData.teams[0].teamNumber != myTeam &&
           robocupData.teams[1].teamNumber != myTeam) {

          cout << "!!NOT OUR GAME!!" << endl;
          return;
        }

        qnode->gameControlData.robotnum = playerNum + 1;
        qnode->gameControlData.position = position;

        qnode->gameControlData.state         = (int)robocupData.state;
        qnode->gameControlData.myside        = /*robocupData.firstHalf ? initSide : !initSide;*/mySide;
        qnode->gameControlData.iskickoff     = (robocupData.kickOffTeam == myTeam) ? true : false;
        qnode->gameControlData.secondstate   = (int)robocupData.secondaryState;
        qnode->gameControlData.readytime = (int)robocupData.secondaryTime;
        qnode->gameControlData.penalty = (int)robocupData.teams[mySide].players[playerNum].penalty;

        qnode->gameControlData.secondinfo.clear();
        for(int i = 0; i < 4; i++)
        {
            qnode->gameControlData.secondinfo.push_back((int)robocupData.secondaryStateInfo[i]);
            std::cout << "secondary info = " << qnode->gameControlData.secondinfo[i] << " " << std::endl;
        }

        mySide = qnode->gameControlData.myside;

        std::cout << endl << endl;

        qnode->gamecontrollerPub->publish(qnode->gameControlData);

        uiUpdate();

    }
    qnode->gameControlData.robotnum=playerNum+1;


}

void MainWindow::uiUpdate()
{
    switch (robocupData.gameType) {
    case GAME_ROUNDROBIN:
        ui->textEdit->append("ROUNDROBIN");
        break;
    case GAME_PLAYOFF:
        ui->textEdit->append("PLAYOFF");
        break;
    case GAME_DROPIN:
        ui->textEdit->append("DROPIN");
        break;
    default:
        break;
    }

    switch (robocupData.firstHalf)
    {
    case 1:
        ui->textEdit->append("1st Half");

        break;

    case 0:
        ui->textEdit->append("2nd Half");
        break;
    default:
        break;
    }

    int secs_remaining = (int)robocupData.secsRemaining;
    if(secs_remaining > 600) {
        secs_remaining = secs_remaining - 65536;
    }
    ui->textEdit->append("secs remaining: " + QString::number(secs_remaining) + "\n");

    if(qnode->gameControlData.myside == LEFT) {
        ui->textEdit->append("side: LEFTSIDE");
    } else {
        ui->textEdit->append("side: RIGHTSIDE");
    }
    ui->checkBox_side->setCheckState(Qt::CheckState::PartiallyChecked);
    ui->checkBox_side->setText("AUTO");

    switch (robocupData.state)
    {
    case STATE_INITIAL:
        ui->textEdit->append("state: INITIAL");
        break;
    case STATE_READY:
        ui->textEdit->append("state: READY");
        ui->textEdit->append("ready time: " + QString::number(qnode->gameControlData.readytime));
        break;
    case STATE_SET:
        ui->textEdit->append("state: SET");
        break;
    case STATE_PLAYING:
        ui->textEdit->append("state: PLAY");
        if(qnode->gameControlData.readytime){
            ui->textEdit->append("kick-off time: " + QString::number(qnode->gameControlData.readytime));
        }
        break;
    case STATE_FINISHED:
        ui->textEdit->append("state: FINISH");
        if(qnode->gameControlData.readytime){
            ui->textEdit->append("half time: " + QString::number(qnode->gameControlData.readytime));
        }
        break;
    default:
        break;
    }
    ui->comboBox_state->setEnabled(false);
    ui->comboBox_state->setCurrentIndex(robocupData.state);

    if(qnode->gameControlData.iskickoff) ui->textEdit->append("kickoff: YES");
    else ui->textEdit->append("kickoff: NO");

    switch (qnode->gameControlData.penalty)
    {
    case HL_BALL_MANIPULATION:
        ui->textEdit->append("penalty: BALL_MANIPULATION");
        break;
    case HL_PHYSICAL_CONTACT:
        ui->textEdit->append("penalty: PHYSICAL_CONTACT");
        break;
    case HL_ILLEGAL_ATTACK:
        ui->textEdit->append("penalty: ILLEGAL_ATTACK");
        break;
    case HL_ILLEGAL_DEFENSE:
        ui->textEdit->append("penalty: ILLEGAL_DEFENSE");
        break;
    case HL_PICKUP_OR_INCAPABLE:
        ui->textEdit->append("penalty: PICKUP_OR_INCAPABLE");
        break;
    case HL_SERVICE:
        ui->textEdit->append("penalty: SERVICE");
        break;
    case SUBSTITUTE:
        ui->textEdit->append("penalty: SUBSTITUTE");
        break;
    default:
        ui->textEdit->append("penalty: NONE");
        break;

    }
    if(qnode->gameControlData.penalty != NONE) {
        ui->textEdit->append("secsTillUnpenalised: " + QString::number((int)robocupData.teams[mySide].players[playerNum].secsTillUnpenalised));
    }

    ui->textEdit->append("warning: " + QString::number((int)robocupData.teams[mySide].players[playerNum].numberOfWarnings));
    ui->textEdit->append("yellow card: " + QString::number((int)robocupData.teams[mySide].players[playerNum].yellowCardCount));
    ui->textEdit->append("red card: " + QString::number((int)robocupData.teams[mySide].players[playerNum].redCardCount));

    switch (qnode->gameControlData.secondstate)
    {
    case STATE2_PENALTYSHOOT:
        ui->textEdit->append("secondary: PENALTYSHOOT");
        ui->textEdit->append("info : " + QString::number(qnode->gameControlData.secondinfo[0]) + " " + QString::number(qnode->gameControlData.secondinfo[1]));
        break;

    case STATE2_OVERTIME:
        ui->textEdit->append("secondary: OVERTIME");
        break;

    case STATE2_TIMEOUT:
        ui->textEdit->append("secondary: TIMEOUT");
        break;

    case STATE2_DIRECT_FREEKICK:
        ui->textEdit->append("secondary: DIRECT_FREEKICK");
        ui->textEdit->append("info: " + QString::number(qnode->gameControlData.secondinfo[0]) + " " + QString::number(qnode->gameControlData.secondinfo[1]));
        break;

    case STATE2_INDIRECT_FREEKICK:
        ui->textEdit->append("secondary: INDIRECT_FREEKICK");
        ui->textEdit->append("info: " + QString::number(qnode->gameControlData.secondinfo[0]) + " " + QString::number(qnode->gameControlData.secondinfo[1]));
        break;

    case STATE2_PENALTYKICK:
        ui->textEdit->append("secondary: PENALTYKICK");
        ui->textEdit->append("info: " + QString::number(qnode->gameControlData.secondinfo[0]) + " " + QString::number(qnode->gameControlData.secondinfo[1]));
        break;

    case STATE2_CORNER_KICK:
        ui->textEdit->append("secondary: CORNERKICK");
        ui->textEdit->append("info: " + QString::number(qnode->gameControlData.secondinfo[0]) + " " + QString::number(qnode->gameControlData.secondinfo[1]));
        break;

    case STATE2_GOAL_KICK:
        ui->textEdit->append("secondary: GOALKICK");
        ui->textEdit->append("info : " + QString::number(qnode->gameControlData.secondinfo[0]) + " " + QString::number(qnode->gameControlData.secondinfo[1]));
        break;

    case STATE2_THROW_IN:
        ui->textEdit->append("secondary: THROWIN");
        ui->textEdit->append("info: " + QString::number(qnode->gameControlData.secondinfo[0]) + " " + QString::number(qnode->gameControlData.secondinfo[1]));
        break;

    default:
        ui->textEdit->append("secondary: NORMAL");
        break;
    }
}

void MainWindow::udpSend_callback()   //send udp data
{
    QByteArray Data;

    if(m_pSendSocket == nullptr){
        m_pSendSocket = new QUdpSocket(this);
    }

    strncpy(robocupreturnData.header, GAMECONTROLLER_RETURN_STRUCT_HEADER, 4);
    robocupreturnData.version       = GAMECONTROLLER_RETURN_STRUCT_VERSION;
    robocupreturnData.team          = static_cast<uint8_t>(myTeam);
    robocupreturnData.player        = static_cast<uint8_t>(playerNum + 1);

    if(position == POSITION_GK) robocupreturnData.message = GAMECONTROLLER_RETURN_MSG_GOALKEEPER;
    else                        robocupreturnData.message = GAMECONTROLLER_RETURN_MSG_ALIVE;

    Data.push_back(static_cast<char>(robocupreturnData.header[0]));
    Data.push_back(static_cast<char>(robocupreturnData.header[1]));
    Data.push_back(static_cast<char>(robocupreturnData.header[2]));
    Data.push_back(static_cast<char>(robocupreturnData.header[3]));
    Data.push_back(static_cast<char>(robocupreturnData.version));
    Data.push_back(static_cast<char>(robocupreturnData.team));
    Data.push_back(static_cast<char>(robocupreturnData.player));
    Data.push_back(static_cast<char>(robocupreturnData.message));

    QHostAddress cntrAddr;
    cntrAddr.setAddress("192.168.0.68");
    m_pSendSocket->writeDatagram(Data.data(),Data.size(), cntrAddr, 3939);
}
