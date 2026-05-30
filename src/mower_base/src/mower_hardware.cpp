#include "mower_base/mower_hardware.hpp"

#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "pluginlib/class_list_macros.hpp"
#include "rclcpp/rclcpp.hpp"

namespace
{
constexpr double TWO_PI = 6.28318530717958647692;
const auto LOGGER = rclcpp::get_logger("mower_base.MowerHardware");
}  // namespace

namespace mower_base
{

MowerHardware::CallbackReturn MowerHardware::on_init(
  const hardware_interface::HardwareComponentInterfaceParams & params)
{
  if (hardware_interface::SystemInterface::on_init(params) != CallbackReturn::SUCCESS)
  {
    return CallbackReturn::ERROR;
  }

  const auto & hw_params = info_.hardware_parameters;
  if (hw_params.count("port") != 0) {
    port_ = hw_params.at("port");
  }
  if (hw_params.count("baud") != 0) {
    baud_ = std::stoi(hw_params.at("baud"));
  }
  if (hw_params.count("counts_per_rev") != 0) {
    counts_per_rev_ = std::stod(hw_params.at("counts_per_rev"));
  }
  if (hw_params.count("max_pwm") != 0) {
    max_pwm_ = std::stoi(hw_params.at("max_pwm"));
  }
  if (hw_params.count("max_wheel_rad_per_sec") != 0) {
    max_wheel_rad_per_sec_ = std::stod(hw_params.at("max_wheel_rad_per_sec"));
  }
  if (hw_params.count("read_timeout_ms") != 0) {
    read_timeout_ms_ = std::stoi(hw_params.at("read_timeout_ms"));
  }

  if (counts_per_rev_ <= 0.0 || max_pwm_ < 0 || max_wheel_rad_per_sec_ <= 0.0) {
    RCLCPP_ERROR(LOGGER, "Invalid mower hardware parameters");
    return CallbackReturn::ERROR;
  }

  if (info_.joints.size() != 2) {
    RCLCPP_ERROR(LOGGER, "Expected exactly 2 wheel joints, got %zu", info_.joints.size());
    return CallbackReturn::ERROR;
  }

  for (const auto & joint : info_.joints) {
    if (joint.command_interfaces.size() != 1 ||
      joint.command_interfaces[0].name != hardware_interface::HW_IF_VELOCITY)
    {
      RCLCPP_ERROR(LOGGER, "Joint %s must expose one velocity command interface", joint.name.c_str());
      return CallbackReturn::ERROR;
    }

    if (joint.state_interfaces.size() != 2) {
      RCLCPP_ERROR(LOGGER, "Joint %s must expose position and velocity state interfaces", joint.name.c_str());
      return CallbackReturn::ERROR;
    }
  }

  return CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface> MowerHardware::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> interfaces;
  interfaces.emplace_back(info_.joints[0].name, hardware_interface::HW_IF_POSITION, &left_position_rad_);
  interfaces.emplace_back(info_.joints[0].name, hardware_interface::HW_IF_VELOCITY, &left_velocity_rad_s_);
  interfaces.emplace_back(info_.joints[1].name, hardware_interface::HW_IF_POSITION, &right_position_rad_);
  interfaces.emplace_back(info_.joints[1].name, hardware_interface::HW_IF_VELOCITY, &right_velocity_rad_s_);
  return interfaces;
}

std::vector<hardware_interface::CommandInterface> MowerHardware::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> interfaces;
  interfaces.emplace_back(info_.joints[0].name, hardware_interface::HW_IF_VELOCITY, &left_command_rad_s_);
  interfaces.emplace_back(info_.joints[1].name, hardware_interface::HW_IF_VELOCITY, &right_command_rad_s_);
  return interfaces;
}

MowerHardware::CallbackReturn MowerHardware::on_activate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  if (!open_serial()) {
    return CallbackReturn::ERROR;
  }

  write_serial_line("BRIDGE_HEARTBEAT");
  send_drive_command(0, 0);
  RCLCPP_INFO(LOGGER, "Activated mower hardware on %s", port_.c_str());
  return CallbackReturn::SUCCESS;
}

MowerHardware::CallbackReturn MowerHardware::on_deactivate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  send_drive_command(0, 0);
  write_serial_line("PWM_OFF");
  close_serial();
  RCLCPP_INFO(LOGGER, "Deactivated mower hardware");
  return CallbackReturn::SUCCESS;
}

hardware_interface::return_type MowerHardware::read(
  const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
  if (serial_fd_ < 0) {
    return hardware_interface::return_type::ERROR;
  }

  if (!write_serial_line("STATE?")) {
    return hardware_interface::return_type::ERROR;
  }

  std::string state_line;
  if (!read_state_line(state_line)) {
    RCLCPP_WARN_THROTTLE(LOGGER, *rclcpp::Clock::make_shared(), 2000, "No STATE response from Arduino");
    return hardware_interface::return_type::OK;
  }

  if (!parse_state_line(state_line)) {
    RCLCPP_WARN(LOGGER, "Could not parse Arduino state line: '%s'", state_line.c_str());
  }

  return hardware_interface::return_type::OK;
}

hardware_interface::return_type MowerHardware::write(
  const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
  if (serial_fd_ < 0) {
    return hardware_interface::return_type::ERROR;
  }

  write_serial_line("BRIDGE_HEARTBEAT");
  const int left_pwm = velocity_to_pwm(left_command_rad_s_);
  const int right_pwm = velocity_to_pwm(right_command_rad_s_);

  if (!send_drive_command(left_pwm, right_pwm)) {
    return hardware_interface::return_type::ERROR;
  }

  return hardware_interface::return_type::OK;
}

bool MowerHardware::open_serial()
{
  serial_fd_ = ::open(port_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (serial_fd_ < 0) {
    RCLCPP_ERROR(LOGGER, "Failed to open %s: %s", port_.c_str(), std::strerror(errno));
    return false;
  }

  if (!configure_serial()) {
    close_serial();
    return false;
  }

  rx_buffer_.clear();
  usleep(2000000);
  return true;
}

void MowerHardware::close_serial()
{
  if (serial_fd_ >= 0) {
    ::close(serial_fd_);
    serial_fd_ = -1;
  }
}

bool MowerHardware::configure_serial()
{
  termios tty {};
  if (tcgetattr(serial_fd_, &tty) != 0) {
    RCLCPP_ERROR(LOGGER, "tcgetattr failed: %s", std::strerror(errno));
    return false;
  }

  speed_t speed = B115200;
  if (baud_ != 115200) {
    RCLCPP_WARN(LOGGER, "Unsupported baud %d requested; using 115200", baud_);
  }

  cfsetospeed(&tty, speed);
  cfsetispeed(&tty, speed);
  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
  tty.c_iflag &= ~IGNBRK;
  tty.c_lflag = 0;
  tty.c_oflag = 0;
  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 0;
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_cflag |= (CLOCAL | CREAD);
  tty.c_cflag &= ~(PARENB | PARODD);
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;

  if (tcsetattr(serial_fd_, TCSANOW, &tty) != 0) {
    RCLCPP_ERROR(LOGGER, "tcsetattr failed: %s", std::strerror(errno));
    return false;
  }

  return true;
}

bool MowerHardware::write_serial_line(const std::string & line)
{
  if (serial_fd_ < 0) {
    return false;
  }

  const std::string framed = line + "\n";
  const ssize_t written = ::write(serial_fd_, framed.data(), framed.size());
  return written == static_cast<ssize_t>(framed.size());
}

bool MowerHardware::read_serial_line(std::string & line, int timeout_ms)
{
  line.clear();
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

  while (std::chrono::steady_clock::now() < deadline) {
    const auto newline = rx_buffer_.find('\n');
    if (newline != std::string::npos) {
      line = rx_buffer_.substr(0, newline);
      rx_buffer_.erase(0, newline + 1);
      line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
      return true;
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(serial_fd_, &read_fds);

    timeval timeout {};
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
    const int ready = select(serial_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
    if (ready > 0 && FD_ISSET(serial_fd_, &read_fds)) {
      char buffer[128];
      const ssize_t received = ::read(serial_fd_, buffer, sizeof(buffer));
      if (received > 0) {
        rx_buffer_.append(buffer, received);
      }
    }
  }

  return false;
}

bool MowerHardware::read_state_line(std::string & state_line)
{
  std::string line;
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(read_timeout_ms_);

  while (std::chrono::steady_clock::now() < deadline) {
    if (!read_serial_line(line, 20)) {
      continue;
    }

    if (line.rfind("STATE ", 0) == 0) {
      state_line = line;
      return true;
    }
  }

  return false;
}

bool MowerHardware::parse_state_line(const std::string & line)
{
  std::istringstream stream(line);
  std::string token;
  std::string left_total_key;
  std::string right_total_key;
  std::string left_hz_key;
  std::string right_hz_key;
  std::string pwm_key;
  unsigned long left_total = 0;
  unsigned long right_total = 0;
  double left_hz = 0.0;
  double right_hz = 0.0;
  int left_pwm = 0;
  int right_pwm = 0;

  stream >> token >> left_total_key >> left_total >> right_total_key >> right_total >>
    left_hz_key >> left_hz >> right_hz_key >> right_hz >> pwm_key >> left_pwm >> right_pwm;

  if (!stream || token != "STATE" || left_total_key != "LEFT_TOTAL" ||
    right_total_key != "RIGHT_TOTAL" || left_hz_key != "LEFT_HZ" ||
    right_hz_key != "RIGHT_HZ" || pwm_key != "PWM")
  {
    return false;
  }

  left_position_rad_ = static_cast<double>(left_total) * TWO_PI / counts_per_rev_;
  right_position_rad_ = static_cast<double>(right_total) * TWO_PI / counts_per_rev_;
  left_velocity_rad_s_ = left_hz * TWO_PI / counts_per_rev_;
  right_velocity_rad_s_ = right_hz * TWO_PI / counts_per_rev_;
  return true;
}

int MowerHardware::velocity_to_pwm(double velocity_rad_s) const
{
  if (velocity_rad_s <= 0.0) {
    return 0;
  }

  const auto scaled = static_cast<int>(std::lround(
    (velocity_rad_s / max_wheel_rad_per_sec_) * static_cast<double>(max_pwm_)));
  return std::clamp(scaled, 0, max_pwm_);
}

bool MowerHardware::send_drive_command(int left_pwm, int right_pwm)
{
  if (left_pwm == last_left_pwm_ && right_pwm == last_right_pwm_) {
    return true;
  }

  std::ostringstream command;
  command << "DRIVE " << left_pwm << " " << right_pwm;
  if (!write_serial_line(command.str())) {
    return false;
  }

  last_left_pwm_ = left_pwm;
  last_right_pwm_ = right_pwm;
  return true;
}

}  // namespace mower_base

PLUGINLIB_EXPORT_CLASS(mower_base::MowerHardware, hardware_interface::SystemInterface)
