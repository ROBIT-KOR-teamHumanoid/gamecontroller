/**
 * @file /include/gamecontroller/qnode.hpp
 *
 * @brief Communications central!
 *
 * @date February 2011
 **/
/*****************************************************************************
** Ifdefs
*****************************************************************************/

#ifndef gamecontroller_QNODE_HPP_
#define gamecontroller_QNODE_HPP_

/*****************************************************************************
** Includes
*****************************************************************************/
#ifndef Q_MOC_RUN
#include <rclcpp/rclcpp.hpp>
#endif
#include <QThread>
#include <string>

#include "humanoid_interfaces/msg/robocupcontroller.hpp"
#include "humanoid_interfaces/msg/gamecontroldata.hpp"
#include "humanoid_interfaces/msg/gamecontrolreturndata.hpp"

/*****************************************************************************
** Class
*****************************************************************************/


class QNode : public QThread
{
  Q_OBJECT
public:
  QNode();
  ~QNode();

  void run();

  humanoid_interfaces::msg::Gamecontroldata gameControlData;
  humanoid_interfaces::msg::Gamecontrolreturndata gameControlReturnData;

  rclcpp::Publisher<humanoid_interfaces::msg::Gamecontroldata>::SharedPtr gamecontrollerPub;

  // config.yaml 파라미터
  std::string pubTopic;
  int teamRobit;
  int teamRobitRed;
  int teamRobitBlue;
  int dataPort;
  int returnPort;

private:
  std::shared_ptr<rclcpp::Node> node;

Q_SIGNALS:
  void rosShutDown();
};

#endif /* gamecontroller_QNODE_HPP_ */
