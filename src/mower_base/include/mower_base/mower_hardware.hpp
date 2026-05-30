#ifndef MOWER_BASE__MOWER_HARDWARE_HPP_
#define MOWER_BASE__MOWER_HARDWARE_HPP_

#include <string>
#include <vector>

#include "hardware_interface/handle.hpp"
#include "hardware_interface/hardware_info.hpp"
#include "hardware_interface/system_interface.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include "rclcpp/time.hpp"
#include "rclcpp_lifecycle/state.hpp"

namespace mower_base
{

class MowerHardware : public hardware_interface::SystemInterface
{
public:
  using CallbackReturn =
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

  CallbackReturn on_init(
    const hardware_interface::HardwareComponentInterfaceParams & params) override;
  CallbackReturn on_activate(const rclcpp_lifecycle::State & previous_state) override;
  CallbackReturn on_deactivate(const rclcpp_lifecycle::State & previous_state) override;

  std::vector<hardware_interface::StateInterface> export_state_interfaces() override;
  std::vector<hardware_interface::CommandInterface> export_command_interfaces() override;

  hardware_interface::return_type read(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;
  hardware_interface::return_type write(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

private:
  bool open_serial();
  void close_serial();
  bool configure_serial();
  bool write_serial_line(const std::string & line);
  bool read_serial_line(std::string & line, int timeout_ms);
  bool read_state_line(std::string & state_line);
  bool parse_state_line(const std::string & line);
  int velocity_to_pwm(double velocity_rad_s) const;
  bool send_drive_command(int left_pwm, int right_pwm);

  std::string port_ = "/dev/ttyACM0";
  int baud_ = 115200;
  double counts_per_rev_ = 205.0;
  int max_pwm_ = 60;
  double max_wheel_rad_per_sec_ = 8.0;
  int read_timeout_ms_ = 80;

  int serial_fd_ = -1;
  std::string rx_buffer_;

  double left_position_rad_ = 0.0;
  double right_position_rad_ = 0.0;
  double left_velocity_rad_s_ = 0.0;
  double right_velocity_rad_s_ = 0.0;
  double left_command_rad_s_ = 0.0;
  double right_command_rad_s_ = 0.0;

  int last_left_pwm_ = -1;
  int last_right_pwm_ = -1;
};

}  // namespace mower_base

#endif  // MOWER_BASE__MOWER_HARDWARE_HPP_
