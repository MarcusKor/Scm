#include <Device.h>

using namespace VS3CODEFACTORY::CORE;

Device::Device()
	: m_eDeviceStatus(DeviceStatus::DeviceNotCreated)
	, m_eDeviceOperation(DeviceOperation::PowerOff)
	, m_eDeviceCommand(DeviceCommand::ByPass)
	, m_eDeviceSignalValueType(DeviceSignalValueType::ComplexValue)
	, m_eDeviceChannelStrucutreType(DeviceChannelStrucutreType::SingleChannel)
	, m_eDeviceSignalDirection(DeviceSignalDirection::Input)
	, m_eDeviceCategory(DeviceCategory::DeviceUndefined)
	, m_eDeviceInterface(DeviceInterface::Interface_Rs232)
	, m_eDeviceInterfaceProtocol(DeviceInterfaceProtocol::Protocol_Custom_Raw)
	, m_nDeviceChannelType(0)
	, m_uId(0)
	, m_strName("") {}

Device::Device(const uint32_t id, const std::string name)
	: Device()
{
	m_uId = id;
	m_strName = name;
}

Device::~Device() {}

IoDevice::IoDevice(const uint32_t id, const std::string name, const DeviceIoType type, const DigitalSignalLevel level)
	: Device(id, name)
	, m_eDigitalSignalLevel(level)
	, m_eDeviceIoType(type)
	, m_dLowLimit(0)
	, m_dHighLimit(0) {}

IoDevice::~IoDevice() {}

void IoDevice::SetPolynomial(const Polynomial& f)
{
	for (auto itr = f.m_vTerms.begin(); itr != f.m_vTerms.end(); itr++)
		m_vFomular.m_vTerms.push_back(new Term(*itr));
}
