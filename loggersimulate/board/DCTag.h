/*******************************************************************************
              Copyright © 1983-2014 Advantech Co., Ltd.
********************************************************************************
THIS IS AN UNPUBLISHED WORK CONTAINING CONFIDENTIAL AND PROPRIETARY INFORMATION
WHICH IS THE PROPERTY OF ADVANTECH CORP., ANY DISCLOSURE, USE, OR REPRODUCTION,
WITHOUT WRITTEN AUTHORIZATION FROM ADVANTECH CORP., IS STRICTLY PROHIBITED.

================================================================================
File:		DCTag.h
Author: 	lin.lei
Created:	7/22/2014
--------------------------------------------------------------------------------
*/

#ifndef DCTAG_H
#define DCTAG_H

/*!
* \file DCTag.h
*
* For using DCTag SDK, please include this file in your source code.
*/


#include <stdint.h>
#include <stdbool.h>
#include "QualityDefine.h"

#ifdef _MSC_VER
#ifdef DCTAG_EXPORTS
#define DCTAG_API __declspec(dllexport)
#else
#define DCTAG_API __declspec(dllimport)
#endif
#else
#define DCTAG_API
#endif

/** The max count of tags when perform read/write operation */
#define DC_TAG_MAX_RW_COUNT				10000

/** The error code of DCTag SDK */
enum dc_tag_error_code
{
	/** At least one given parameter is invalid */
	DC_TAG_ERR_INVALID_PARAM			= -1,
	/** Has not connected to the DataCenter */
	DC_TAG_ERR_NO_CONNECTION			= -2,
	/** Exception when allocation memory */
	DC_TAG_ERR_MEM_ALLOC_FAILED			= -3,
	/** Fail to send request to the DataCenter */
	DC_TAG_ERR_REQUEST_FAILED			= -4,
	/** The DataCenter return value count not equals to the request count */
	DC_TAG_ERR_VALUE_COUNT_ERROR		= -5,
	/** The DataCenter return unexpected value type */
	DC_TAG_ERR_VALUE_TYPE_ERROR			= -6,
	/** Fail to initialize the thread */
	DC_TAG_ERR_THREAD_ERROR				= -7,
	/** The input buffer size is too small */
	DC_TAG_ERR_INBUF_TOO_SMALL			= -8,
	/** The data center is too busy to handle the request */
	DC_TAG_ERR_DATA_CENTER_BUSY			= -9,
	/** The SDK has not initialized */
	DC_TAG_ERR_HAS_NOT_INIT				= -10,
};

#define DCU_CAPABILITY_READ				1
#define DCU_CAPABILITY_WRITE			2
#define DCU_CAPABILITY_UPDATE			4

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

	/** The handle type for tag */
	typedef void *						DC_TAG_HANDLE;

	/** The tag data struct */
	struct dc_tag
	{
		/** Tag value */
		double value;

		/** Tag quality, negative value mean exception */
		int16_t quality;

		/** Tag time stamp, in UTC format */
	    uint64_t time;

		/** Tag time stamp, microsecond part */
	    uint32_t usec;
	};
	/** typedef of struct dc_tag */
	typedef struct dc_tag				DC_TAG;

	/** Callback functions */
	struct dc_tag_callback
	{
		/** the byte size of this struct */
		uint32_t size;

		/** read_tag */
		int (*read_tag) (
			DC_TAG_HANDLE tag_handles[],
			void * tag_contexts[],
			DC_TAG tag_values[],
			uint32_t tag_count,
			void * user_context );

		/** write_tag */
		int (*write_tag) (
			DC_TAG_HANDLE tag_handles[],
			void * tag_contexts[],
			double tag_values[],
			uint32_t tag_count,
			void * user_context );

		/** update_tag */
		int (*update_tag) (
			DC_TAG_HANDLE tag_handle,
			void * tag_context,
			DC_TAG * tag_value,
			void * user_context );
	};
	/** typedef of struct dc_tag_callback */
	typedef struct dc_tag_callback		DC_TAG_CALLBACK;

	/*!*************************************************************************
	* \brief
	*	Initialize the DCTag SDK.
	*
	* \details
	*	Before any other API can be used correctly, you must call this function
	*	once and only once to perform a runtime initialization firstly.
	*
	* \param[in] max_tag_count
	*	the max tag count you will handle in your application.
	* \param[in] callback
	*	the callback function pointer, in struct \ref dc_tag_callback. If your
	*	application will not add any tag to the DataCenter, you can ignore this
	*	parameter and give a NULL value.
	* \param[in] user_context
	*	the context pointer, which will be passed in the callback functions.
	*
	* \return
	*    On success, zero is returned, otherwise an error code is returned.
	*    please refer to the enum \ref dc_tag_error_code for the error code
	*    description.
	*
	* \see dc_tag_callback
	***************************************************************************/
	DCTAG_API int dc_tag_init (
		uint32_t max_tag_count,
		DC_TAG_CALLBACK * callback,
		void * user_context );

	/*!*************************************************************************
	* \brief
	*	Uninitialize the DCTag SDK.
	*
	* \details
	*	Call this function before application exit to release all resources
	*	allocated by the DCTag SDK.
	***************************************************************************/
	DCTAG_API void dc_tag_uninit ( void );

	/*!*************************************************************************
	* \brief
	*	Add a tag to the DataCenter.
	*
	* \details
	*
	* \param[in] tag_name
	*	Specify the name of the tag you want to add to the DataCenter, if an
	*	exist tag in the DataCenter has the same name with the tag adding, then
	*	this function will return error and no new tag will be added to the
	*	DataCenter.
	* \param[in] tag_context
	*	The context related to the tag added, it will be passed by the read or
	*	write callback function.
	*
	* \return
	*    On success, a valid tag handle is returned, otherwise NULL is returned.
	***************************************************************************/
	DCTAG_API DC_TAG_HANDLE dc_tag_add (
		char const * tag_name,
		void * tag_context );

	/*!*************************************************************************
	* \brief
	*	Remove a tag from the DataCenter which previous added by yourself.
	*
	* \details
	*	When calling this function, please make sure the tag be removed is the
	*	tag added by \ref dc_tag_add previous, not the tag opened.
	*
	* \param[in] tag_handle
	*	The tag handle returned by a successful call of the \ref dc_tag_add.
	*
	* \return
	*    On success, zero is returned, otherwise an error code is returned,
	*    please refer to the enum \ref dc_tag_error_code for the error code
	*    description.
	***************************************************************************/
	DCTAG_API int dc_tag_remove ( DC_TAG_HANDLE tag_handle );

	/*!*************************************************************************
	* \brief
	*	Open a exist tag in the DataCenter with the specified name.
	*
	* \details
	*	Even the tag would be opened is not present in the DataCenter, this
	*	function also return a valid handle, but if you read the tag you would
	*	get a exception quality code.
	*
	* \param[in] tag_name
	*	Specify the name of the tag you want to add to the DataCenter.
	* \param[in] tag_context
	*	The context related to the tag opened, it will be passed by the update
	*	callback function.
	*
	* \return
	*    On success, a valid tag handle is returned, otherwise NULL is returned.
	***************************************************************************/
	DCTAG_API DC_TAG_HANDLE dc_tag_open (
		char const * tag_name,
		void * tag_context );

	/*!*************************************************************************
	* \brief
	*	Close a tag from the DataCenter which previous opened by yourself.
	*
	* \details
	*	When calling this function, please make sure the tag be closed is the
	*	tag added by \ref dc_tag_open previous, not the tag added.
	*
	* \param[in] tag_handle
	*	The tag handle returned by a successful call of the \ref dc_tag_open.
	*
	* \return
	*    On success, zero is returned, otherwise an error code is returned.
	*    please refer to the enum \ref dc_tag_error_code for the error code
	*    description.
	***************************************************************************/
	DCTAG_API int dc_tag_close ( DC_TAG_HANDLE tag_handle );

	/*!*************************************************************************
	* \brief
	*	Subscribe tags for read.
	*
	* \details
	*	If you need to read some tags periodically, you can subscribe these tags
	*	before read and call \ref dc_tag_read_subscription to read tag values,
	*	this will achieve the best reading performance.
	*
	* \param[in] tag_handles
	*	The tag handle array contains all tag handles to be read.
	* \param[in] tag_count
	*	The count of the tag handle in the tag_handles.
	*
	* \return
	*    On success, a subscription context will returned, otherwise a NULL is
	*    returned.
	***************************************************************************/
	DCTAG_API void * dc_tag_add_subscription (
		DC_TAG_HANDLE tag_handles[],
		uint32_t tag_count );

	/*!*************************************************************************
	* \brief
	*	Remove a previous subscription.
	*
	* \details
	*	When you finished all operation on a subscription, you can call this
	*	function to remove the subscription from the Data Center.
	*
	* \param[in] subscription
	*	The subscription context returned by a successful call to the \ref
	*	dc_tag_add_subscription.
	*
	* \return
	*    On success, zero is returned, otherwise an error code is returned.
	*    please refer to the enum \ref dc_tag_error_code for the error code
	*    description.
	***************************************************************************/
	DCTAG_API int dc_tag_remove_subscription ( void * subscription );

	/*!*************************************************************************
	* \brief
	*	Read one or more tags by previous subscription.
	*
	* \details
	*	Used together with the \ref dc_tag_add_subscription function to
	*	complete a read operation.
	*
	* \param[in] subscription
	*	The subscription context returned by a successful call to the \ref
	*	dc_tag_add_subscription.
	* \param[out] tag_values
	*	The tag value array contains all tag value returned, be sure the count
	*	of the tag value equals to or larger than the count of the tag handle.
	* \param[in] tag_count
	*	The count of the tag values in the tag_values.
	*
	* \return
	*    On success, zero is returned, otherwise an error code is returned.
	*    please refer to the enum \ref dc_tag_error_code for the error code
	*    description.
	***************************************************************************/
	DCTAG_API int dc_tag_read_subscription (
		void * subscription,
		DC_TAG tag_values[],
		uint32_t tag_count );

	/*!*************************************************************************
	* \brief
	*	Read one or more tags.
	*
	* \details
	*	Before perform read operation, you should prepare a tag handle array,
	*	which contains all tag handles to be read, be note the max tag count is
	*	limited by \ref DC_TAG_MAX_RW_COUNT.
	*
	* \param[in] tag_handles
	*	The tag handle array contains all tag handles to be read.
	* \param[out] tag_values
	*	The tag value array contains all tag value returned, be sure the count
	*	of the tag value equals to or larger than the count of the tag handle.
	* \param[in] tag_count
	*	The count of the tag handle in the tag_handles.
	*
	* \return
	*    On success, zero is returned, otherwise an error code is returned.
	*    please refer to the enum \ref dc_tag_error_code for the error code
	*    description.
	*
	* \remark
	*	If there is only one tag, be sure to use the pointer of the tag handle
	*	instead of using the tag handle directly.
	***************************************************************************/
	DCTAG_API int dc_tag_read (
		DC_TAG_HANDLE tag_handles[],
		DC_TAG tag_values[],
		uint32_t tag_count );

	/*!*************************************************************************
	* \brief
	*	Write one or more tags.
	*
	* \details
	*	Before perform write operation, you should prepare a tag handle array,
	*	which contains all tag handles to be written, be note the max tag count
	*	is limited by \ref DC_TAG_MAX_RW_COUNT.
	*
	* \param[in] tag_handles
	*	The tag handle array contains all tag handles to be written.
	* \param[out] tag_values
	*	The tag value array contains all tag value to be written, be sure the
	*	count of the tag value equals to or larger than the count of the tag
	*	handle.
	* \param[in] tag_count
	*	The count of the tag handle in the tag_handles.
	*
	* \return
	*    On success, zero is returned, otherwise an error code is returned.
	*    please refer to the enum \ref dc_tag_error_code for the error code
	*    description.
	***************************************************************************/
	DCTAG_API int dc_tag_write (
		DC_TAG_HANDLE tag_handles[],
		double tag_values[],
		uint32_t tag_count );

	/*!*************************************************************************
	* \brief
	*	Update one or more tags.
	*
	* \details
	*	This function is called by tag provider only. Whenever the value or
	*	quality of tags added by the tag provider changed, the tag provider
	*	should call this function to make the changes pushed to all data users
	*	through the data center.
	*
	* \param[in] tag_handles
	*	The tag handle array contains all tag handles to be updated.
	* \param[out] tag_values
	*	The tag value array contains all tag value to be updated, be sure the
	*	count of the tag value equals to or larger than the count of the tag
	*	handle.
	* \param[in] tag_count
	*	The count of the tag handle in the tag_handles.
	*
	* \return
	*    On success, zero is returned, otherwise an error code is returned.
	*    please refer to the enum \ref dc_tag_error_code for the error code
	*    description.
	***************************************************************************/
	DCTAG_API int dc_tag_update (
		DC_TAG_HANDLE tag_handles[],
		DC_TAG tag_values[],
		uint32_t tag_count );

	/*!*************************************************************************
	* \brief
	*	obtain explanations for the specified quality value.
	*
	* \details
	*	This function is used to obtain an explanation of the quality value of a tag.
	*
	* \param[in] quality
	*	The quality value to be explained.
	*
	* \return
	*   The explain text to the quality.
	***************************************************************************/
	DCTAG_API char const * dc_tag_explain_quality ( int16_t quality );

	/*!*************************************************************************
	* \example dc_tag_owner.c
	*	This is an example of how to add tags to the DataCenter.
	*	This example will add three tags into the DataCenter: tag_1, tag_2 and
	*	tag_3. The tag_1 and the tag_2 can be write, but the tag_3 can only be
	*	read, the value of the tag_3 is calculated by tag_1 * tag_2.
	*
	* \example dc_tag_read.c
	*	This is an example of how to read tags in the DataCenter.
	*
	* \example dc_tag_read_subscription.c
	*	This is an example of how to read tags in the DataCenter by subscription.
	*
	* \example dc_tag_write.c
	*	This is an example of how to write tag in the DataCenter.
	*
	* \example dc_tag_update.c
	*	This is an example of how to add and update tags in the DataCenter.
	*	This example will add three tags into the DataCenter: tag_1, tag_2 and
	*	tag_3. The tag_1 and the tag_2 are initialized with a random value, and
	*	added up every 10 seconds. The tag_3 is calculated by tag_1 + tag_2.
	***************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
