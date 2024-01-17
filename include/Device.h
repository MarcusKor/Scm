#pragma once

#ifndef DEVICEOBJECT_H
#define DEVICEOBJECT_H

#include <cppstd.h>
#include <Polynomial.h>

namespace VS3CODEFACTORY::CORE
{
	typedef enum eDeviceStatus
	{
		OccurredReceiveFailure = -6,
		OccurredTransmitFailure = -5,
		OccurredReceiveTimedout = -4,
		OccurredTransmitTimedout = -3,
		OccurredOpenFailure = -2,
		DeviceHaltedByError = -1,
		DeviceNotCreated = 0,
		DeviceCreated,
		DeviceUninitialized,
		DeviceInitialized,
		DeviceStarted,
		DeviceRunning,
		DeviceOpened,
		DeviceClosed,
		DeviceStopped,
		DeviceDestroyed
	} DeviceStatus;

	typedef enum eDeviceOperation
	{
		PowerOff,
		PowerOn,
		Initialize,
		Home,
		ManualRun,
		CycleStepRun,
		Run,
		Pause,
		Stop,
		Reset,		
	} DeviceOperation;


	typedef enum eDeviceCommand
	{
		ByPass,
		Read,
		ReadWithinTimeout,
		ReadAsync,
		Write,
		WriteWithinTimeout,
		WriteAsync		
	} DeviceCommand;

	typedef enum eDeviceMoveDirection
	{
		MoveToPositivie,
		MoveToNegative
	} DeviceMoveDirection;

	typedef enum eDeviceMoveLocation
	{
		PositiveErrorLimit,
		PositiveWarningLimit,
		PositiveSoftErrorLimit,
		PositiveSoftWarningLimit,
		HomePosition,
		FirstSensorPosition,
		SecondSensorPosition,
		OffsetPosition,
		NegativeSoftWarningLimit,
		NegativeSoftErrorLimit,
		NegativeWarningLimit,
		NegativeErrorLimit
	} DeviceMoveLocation;

	typedef enum eDeviceSignalValueType
	{
		ComplexValue,
		Counter,
		Relay,
		Trigger,
		Current,
		Voltage,
		Frequency,
		Velocity,
		Acceleration,
		Deceleration,
		Rpm,
		Angle,
		Temperature,
		Humidity,
		Presure,
		Vacuum
	} DeviceSignalValueType;

	typedef enum eDigitalSignalLevel
	{
		TtlSignalLevel,
		PulseSignalLevel,
		PwmSignalLevel
	} DigitalSignalLevel;

	typedef enum eDeviceChannelStructureType
	{
		SingleChannel,
		MultipleChannel
	} DeviceChannelStrucutreType;

	typedef enum eDeviceChannelType
	{
		DigitalInputChannel = 0x01,
		DigitalOutputChannel = 0x02,
		CounterChannel = 0x04,
		TriggerChannel = 0x08,
		AnalogInputChannel = 0x10,
		AnalogOutputChannel = 0x20,
		ExternalTimerChannel = 0x40,
		AuxiliaryChannel = 0x80
	} DeviceChannelType;

	typedef enum eDeviceSignalDirection
	{
		Input,
		Output,
		InputOut
	} DeviceSignalDirection;

	typedef enum eDeviceIoType
	{
		CommunicationIo,
		Daq,
		DigitalIo,
		AnalogIo
	} DeviceIoType;

	typedef enum eDeviceCategory
	{
		DeviceUndefined,
		IoDevice,
		MotionDevice,
		AxisDevice,
		StageDevice,
		PowerSupplyDevice,
		PowerControlDevice,
		OpticalDevice,
		LaserDevice,
		ImagingDevice,
		PrintDevice,
		DetectionDevice,
		MeasuringDevice,
		ActuatorDevice,
		SubController,
		OtherRemoteController,
	} DeviceCategory;

	typedef enum eDeviceInterface
	{
		Interface_Rs232,
		Interface_Rs422,
		Interface_Rs485,
		Interface_I2c,
		Interface_Uart,
		Interface_Usb,
		Interface_Bluetooth,
		Interface_Wifi,
		Interface_Ethernet,
		Interface_Parallel,
		Interface_Pci,
		Interface_Vme,
		Interface_Gpio,
		Interface_FieldBus		
	} DeviceInterface;

	typedef enum eDeviceInterfaceProtocol
	{
		Protocol_Custom_Raw,
		Protocol_Custom_Xml,
		Protocol_Custom_Json,
		Protocol_Custom_Restful,
		Protocol_ModbusAscii,
		Protocol_ModbusRtu,
		Protocol_ModbusRtuOverTcp,
		Protocol_ModbusTcp,
		Protocol_CanBus,
		Protocol_LanBus,
		Protocol_Ethercat,
		Protocol_Protobuf,
		Protocol_Secs,
		Protocol_Secs_Gem,
		Protocol_Secs_Gem300
	} DeviceInterfaceProtocol;

	class Device
		: public std::enable_shared_from_this<Device>
	{
	public:
		uint32_t m_uId;
		std::string m_strName;

		Device(const Device& src) = delete;
		Device& operator=(const Device& src) = delete;

		Device();
		Device(const uint32_t id, const std::string name);
		virtual ~Device();

		virtual DeviceStatus GetStatus() const { return m_eDeviceStatus; }
		virtual void SetStatus(DeviceStatus status) { m_eDeviceStatus = status; }
		virtual DeviceOperation GetOperation() { return m_eDeviceOperation; }
		virtual void SetOperation(DeviceOperation status) { m_eDeviceOperation = status; }
		virtual DeviceCommand GetCommand() const { return m_eDeviceCommand; }
		virtual void SetCommand(DeviceCommand cmd) { m_eDeviceCommand = cmd; }
		virtual DeviceSignalValueType GetSignalValueType() const { return m_eDeviceSignalValueType; }
		virtual void SetSignalValueType(DeviceSignalValueType type) { m_eDeviceSignalValueType = type; }
		virtual DeviceChannelStrucutreType GetChannelStructureType() const { return m_eDeviceChannelStrucutreType; }
		virtual void SetChannelStructureType(DeviceChannelStrucutreType type) { m_eDeviceChannelStrucutreType = type; }
		virtual DeviceSignalDirection GetSignalDirection() const { return m_eDeviceSignalDirection; }
		virtual void SetSignalDirection(DeviceSignalDirection type) { m_eDeviceSignalDirection = type; }
		virtual DeviceCategory GetCategory() const { return m_eDeviceCategory; }
		virtual void SetCategory(DeviceCategory status) { m_eDeviceCategory = status; }
		virtual DeviceInterface GetInterfaceType() const { return m_eDeviceInterface; }
		virtual void SetInterfaceType(DeviceInterface type) { m_eDeviceInterface = type; }
		virtual DeviceInterfaceProtocol GetProtocol() const { return m_eDeviceInterfaceProtocol; }
		virtual void SetProtocol(DeviceInterfaceProtocol type) { m_eDeviceInterfaceProtocol = type; }
		virtual uint32_t GetChannelType() const { return m_nDeviceChannelType; }
		virtual void SetChannelType(DeviceChannelType type) { m_nDeviceChannelType |= type; }

	protected:
		DeviceStatus m_eDeviceStatus;
		DeviceOperation m_eDeviceOperation;
		DeviceCommand m_eDeviceCommand;
		DeviceSignalValueType m_eDeviceSignalValueType;
		DeviceChannelStrucutreType m_eDeviceChannelStrucutreType;
		DeviceSignalDirection m_eDeviceSignalDirection;
		DeviceCategory m_eDeviceCategory;
		DeviceInterface m_eDeviceInterface;
		DeviceInterfaceProtocol m_eDeviceInterfaceProtocol;
		uint32_t m_nDeviceChannelType;
	};

	class IoDevice
		: public Device
	{
	public:
		IoDevice(const uint32_t id, const std::string name, const DeviceIoType type = DeviceIoType::DigitalIo, const DigitalSignalLevel level = DigitalSignalLevel::TtlSignalLevel);
		virtual ~IoDevice();

		virtual DigitalSignalLevel GetSignalLevel() const { return m_eDigitalSignalLevel; }
		virtual void SetSignalLevel(DigitalSignalLevel type) { m_eDigitalSignalLevel = type; }
		virtual DeviceIoType GetIoType() const { return m_eDeviceIoType; }
		virtual void SetIoType(DeviceIoType type) { m_eDeviceIoType = type; }
		virtual void GetRange(double_t& low, double_t& high) { low = m_dLowLimit; high = m_dHighLimit; }
		virtual void SetRange(const double_t low, const double_t high) { m_dLowLimit = low; m_dHighLimit = high; }
		virtual Polynomial GetPolynomial() { return m_vFomular; }
		virtual void SetPolynomial(const Polynomial& f);

	protected:
		double_t m_dLowLimit;
		double_t m_dHighLimit;
		Polynomial m_vFomular;
		DigitalSignalLevel m_eDigitalSignalLevel;
		DeviceIoType m_eDeviceIoType;
	};

	class CommunicationIoDevice
		: public IoDevice
	{
	public:
		CommunicationIoDevice();
		CommunicationIoDevice(const uint32_t id, const std::string name);
		virtual ~CommunicationIoDevice();
	};

	class MotionDevice
		: public CommunicationIoDevice
	{
	public:
		MotionDevice();
		MotionDevice(const uint32_t id, const std::string name);
		virtual ~MotionDevice();

		virtual DeviceMoveDirection GetMoveDirection() const { return m_eDeviceMoveDirection; }
		virtual void SetMoveDirection(DeviceMoveDirection type) { m_eDeviceMoveDirection = type; }
		virtual DeviceMoveLocation GetMoveLocation() const { return m_eDeviceMoveLocation; }
		virtual void SetMoveLocation(DeviceMoveLocation type) { m_eDeviceMoveLocation = type; }

	protected:
		DeviceMoveDirection m_eDeviceMoveDirection;
		DeviceMoveLocation m_eDeviceMoveLocation;
	};
}

#endif // DEVICEOBJECT_H