/****************************************************************************
 *
 *   Copyright (c) 2021 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file Gyro.hpp
 *
 * Defines basic functionality of UAVCAN v1 Gyro publisher
 *
 * @author Shaun Cosgrove <shaun@flyvoly.com>
 */

#pragma once

// DS-15 Specification Messages
#include <reg/drone/physics/kinematics/rotation/Planar_0_1.h>
#include <reg/drone/physics/kinematics/cartesian/Pose_0_1.h>
#include <reg/drone/service/battery/Parameters_0_3.h>

#include "Publisher.hpp"

#include <uORB/topics/sensor_gyro.h>

class UavcanDummyPublisherGeneric
{
	protected:
	unsigned long _counter {0};
	float _value = {0};
	unsigned long _rate {100};

	int messages_this_tick(const char* param_name) {
		int result = -1;
		// update the params
		param_get(param_find(param_name), &_value);

		_counter++;

		// Must be greater than zero and below a high number
		if(_value<=0 || _value>2000)
			return result;

		// Convert to target value
		float target = (float)_rate / _value;
		if(target < (float)1.0) {
			// Target is less than 1 meaning more than one
			// message per tick, invert it
			result = floor((float)1.0/target);
		} else {
			// Less than 1 message per tick, is this the tick
			// to send a message?
			result = _counter%(int)floor(target) == 0;
		}
		// PX4_INFO("Counter %lu Value %f [%d] Result %d", _counter,  (double)_value, target < (float)1.0, result );
		return result;
	}

	int get_messages_to_tx(uint16_t port_id, const char* param_name){
		int msg_to_send = -1;
		if (port_id != CANARD_PORT_ID_UNSET && port_id != 0) { //FIXME either make default param UNSET or handle 0 in base class
			msg_to_send = messages_this_tick(param_name);
			if(msg_to_send <= 0){
				msg_to_send = -1;
			}
		}
		return msg_to_send;
	}

};

class UavcanDummyPublisherSmall : public UavcanPublisher, UavcanDummyPublisherGeneric
{
public:
	UavcanDummyPublisherSmall(CanardInstance &ins, UavcanParamManager &pmgr, uint8_t instance = 0) :
		UavcanPublisher(ins, pmgr, "dummy.sml", instance)
	{

	};

	void gen_message() {
		// Use the gryo data and the associated rotation message as the small one
		sensor_gyro_s gyro {};
		_gyro_sub.update(&gyro);

		reg_drone_physics_kinematics_rotation_Planar_0_1 rs{};
		rs.angular_acceleration.radian_per_second_per_second = gyro.x;

		uint8_t gyro_payload_buffer[reg_drone_physics_kinematics_rotation_Planar_0_1_SERIALIZATION_BUFFER_SIZE_BYTES_];

		CanardTransfer transfer = {
			.timestamp_usec = hrt_absolute_time() + PUBLISHER_DEFAULT_TIMEOUT_USEC,
			.priority       = CanardPriorityNominal,
			.transfer_kind  = CanardTransferKindMessage,
			.port_id        = _port_id, // This is the subject-ID.
			.remote_node_id = CANARD_NODE_ID_UNSET,
			.transfer_id    = _transfer_id,
			.payload_size   = reg_drone_physics_kinematics_rotation_Planar_0_1_SERIALIZATION_BUFFER_SIZE_BYTES_,
			.payload        = &gyro_payload_buffer,
		};

		int32_t result = reg_drone_physics_kinematics_rotation_Planar_0_1_serialize_(&rs, gyro_payload_buffer,
					&transfer.payload_size);

		if (result == 0) {
			// set the data ready in the buffer and chop if needed
			++_transfer_id;  // The transfer-ID shall be incremented after every transmission on this subject.
			result = canardTxPush(&_canard_instance, &transfer);
		}
	}

	// Update the uORB Subscription and broadcast a UAVCAN message
	virtual void update() override
	{
		int n = get_messages_to_tx(_port_id,"UCAN1_DPUB_SM_HZ");

		if(n <= 0){
			return;
		}

		for(int i=0;i<n;i++){
			gen_message();
		}

	};

private:

	uORB::Subscription _gyro_sub{ORB_ID(sensor_gyro)};
	CanardTransferID _transfer_id_2 {0};
};


class UavcanDummyPublisherLrg : public UavcanPublisher, UavcanDummyPublisherGeneric
{
public:
	UavcanDummyPublisherLrg(CanardInstance &ins, UavcanParamManager &pmgr, uint8_t instance = 0) :
		UavcanPublisher(ins, pmgr, "dummy.lrg", instance)
	{

	};

	void gen_message() {

		reg_drone_service_battery_Parameters_0_3 rs{};

		uint8_t gyro_payload_buffer[reg_drone_service_battery_Parameters_0_3_SERIALIZATION_BUFFER_SIZE_BYTES_];

		CanardTransfer transfer = {
			.timestamp_usec = hrt_absolute_time() + PUBLISHER_DEFAULT_TIMEOUT_USEC,
			.priority       = CanardPriorityNominal,
			.transfer_kind  = CanardTransferKindMessage,
			.port_id        = _port_id, // This is the subject-ID.
			.remote_node_id = CANARD_NODE_ID_UNSET,
			.transfer_id    = _transfer_id,
			.payload_size   = reg_drone_service_battery_Parameters_0_3_SERIALIZATION_BUFFER_SIZE_BYTES_,
			.payload        = &gyro_payload_buffer,
		};

		int32_t result = reg_drone_service_battery_Parameters_0_3_serialize_(&rs, gyro_payload_buffer,
					&transfer.payload_size);

		if (result == 0) {
			// set the data ready in the buffer and chop if needed
			++_transfer_id;  // The transfer-ID shall be incremented after every transmission on this subject.
			result = canardTxPush(&_canard_instance, &transfer);
		}
	}

	// Update the uORB Subscription and broadcast a UAVCAN message
	virtual void update() override
	{
		int n = get_messages_to_tx(_port_id,"UCAN1_DPUB_LG_HZ");

		if(n <= 0){
			return;
		}

		for(int i=0;i<n;i++){
			gen_message();
		}

	};

private:

	CanardTransferID _transfer_id_2 {0};
};


class UavcanDummyPublisherMed : public UavcanPublisher, UavcanDummyPublisherGeneric
{
public:
	UavcanDummyPublisherMed(CanardInstance &ins, UavcanParamManager &pmgr, uint8_t instance = 0) :
		UavcanPublisher(ins, pmgr, "dummy.med", instance)
	{

	};


	void gen_message() {

		reg_drone_physics_kinematics_geodetic_Point_0_1 rs{};

		uint8_t gyro_payload_buffer[reg_drone_physics_kinematics_geodetic_Point_0_1_SERIALIZATION_BUFFER_SIZE_BYTES_];

		CanardTransfer transfer = {
			.timestamp_usec = hrt_absolute_time() + PUBLISHER_DEFAULT_TIMEOUT_USEC,
			.priority       = CanardPriorityNominal,
			.transfer_kind  = CanardTransferKindMessage,
			.port_id        = _port_id, // This is the subject-ID.
			.remote_node_id = CANARD_NODE_ID_UNSET,
			.transfer_id    = _transfer_id,
			.payload_size   = reg_drone_physics_kinematics_geodetic_Point_0_1_SERIALIZATION_BUFFER_SIZE_BYTES_,
			.payload        = &gyro_payload_buffer,
		};

		int32_t result = reg_drone_physics_kinematics_geodetic_Point_0_1_serialize_(&rs, gyro_payload_buffer,
					&transfer.payload_size);

		if (result == 0) {
			// set the data ready in the buffer and chop if needed
			++_transfer_id;  // The transfer-ID shall be incremented after every transmission on this subject.
			result = canardTxPush(&_canard_instance, &transfer);
		}
	}

		// Update the uORB Subscription and broadcast a UAVCAN message
	virtual void update() override
	{
		int n = get_messages_to_tx(_port_id,"UCAN1_DPUB_MD_HZ");

		if(n <= 0){
			return;
		}

		for(int i=0;i<n;i++){
			gen_message();
		}

	};

private:

	CanardTransferID _transfer_id_2 {0};
};
