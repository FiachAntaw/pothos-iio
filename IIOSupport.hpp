// Copyright (c) 2016 Fiach Antaw
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <iio.h>
#include <memory>
#include <Poco/SingletonHolder.h>
#include <string>
#include <vector>
#include <iterator>

template <class T>
class IIOAttr;
class IIOBuffer;
class IIOChannel;
class IIODevice;

/*!
 * IIOContextRaw contains a raw iio_context pointer, which it destroys
 * automatically when it's destructor is called.
 */
class IIOContextRaw
{
    friend class IIOContext;
private:
    struct iio_context *raw_ptr;

    IIOContextRaw(void);

public:
    ~IIOContextRaw(void);
};

/*!
 * IIOContext represents a libiio context object.
 */
class IIOContext
{
    friend class Poco::SingletonHolder<IIOContext>;
private:
    std::shared_ptr<IIOContextRaw> ctx;

    IIOContext(void);

public:
    /*!
     * Get the global instance of the IIOContext object.
     */
    static IIOContext& get();

    /*!
     * Get the version of the linked IIO library.
     */
    std::string version(void);

    /*!
     * Get the name of the given context.
     */
    std::string name(void);

    /*!
     * Get a description of the given context.
     */
    std::string description(void);

    /*!
     * The devices() method returns a set of IIODevice objects representing
     * devices available through this libiio context.
     */
    std::vector<IIODevice> devices(void);
};

/*!
 * IIOAttrs is a map-like representation of attributes exposed by libiio.
 */
template <class T>
class IIOAttrs
{
    friend T;
private:
    T parent;
    IIOAttrs<T>(T parent);
public:
    class Iterator
    {
        friend class IIOAttrs<T>;
    private:
        T parent;
        unsigned int idx;
        Iterator(T parent, unsigned int idx = 0);
    public:
        Iterator& operator++()
        {
            idx++;
            return *this;
        }
        Iterator operator++(int)
        {
            Iterator retval = *this;
            ++(*this);
            return retval;
        }
        bool operator==(Iterator other) const
        {
            return (this->parent == other.parent) && (this->idx == other.idx);
        }
        bool operator!=(Iterator other) const
        {
            return !(*this == other);
        }
        IIOAttr<T> operator*();
        using difference_type = unsigned int;
        using value_type = IIOAttr<T>;
        using pointer = const IIOAttr<T>*;
        using reference = const IIOAttr<T>&;
        using iterator_category = std::forward_iterator_tag;
    };

    Iterator begin();
    Iterator end();

    IIOAttr<T> at(const std::string& name);
    size_t size() const;
    bool empty() const;
};

/*!
 * Represents a single attribute on a libiio object.
 */
template <class T>
class IIOAttr
{
    friend class IIOAttrs<T>::Iterator;
private:
    IIOAttr<T>(T parent, const char* attr);
    T parent;
    const char* attr;
public:
    /*!
     * Get the name of the attribute.
     */
    std::string name();

    /*!
     * Get the value of the attribute.
     */
    std::string value();

    IIOAttr<T>& operator= (const std::string& other);
    operator std::string() const;
};

/*!
 * IIODevice represents an IIO device exposed via libiio.
 */
class IIODevice
{
    friend class IIOAttr<IIODevice>;
    friend class IIOAttrs<IIODevice>;
    friend class IIOBuffer;
    friend class IIOChannel;
    friend class IIOContext;
private:
    std::shared_ptr<IIOContextRaw> ctx;
    const struct iio_device *device;

    IIODevice(std::shared_ptr<IIOContextRaw> ctx, const struct iio_device *device);

    const char * iio_get_attr(unsigned int idx) const;
    unsigned int iio_get_attrs_count() const;
    ssize_t iio_attr_read(const char *attr, char *dst, size_t len) const;
    ssize_t iio_attr_write(const char *attr, const char *src) const;
public:

    bool operator==(IIODevice other) const
    {
        return this->device == other.device;
    }

    bool operator!=(IIODevice other) const
    {
        return !(*this == other);
    }

    /*!
     * Get the ID of this IIO device.
     */
    std::string id(void);

    /*!
     * Get the name of this IIO device, or "<unnamed>" if the device has no name.
     */
    std::string name(void);

    /*!
     * The attributes() method returns an object exposing attributes available to
     * be read or set on this IIO device.
     */
    IIOAttrs<IIODevice> attributes(void);

    /*!
     * The channels() method returns a set of IIOChannel objects representing
     * channels available on this IIO device.
     */
    std::vector<IIOChannel> channels(void);

    /*!
     * Get an IIODevice object representing the trigger device associated with
     * this device.
     *
     * If no trigger device is associated with this device, a Pothos::NotFound
     * exception will be thrown.
     */
    IIODevice trigger(void);

    /*!
     * Set the trigger device associated with this device.
     * If trigger is NULL, no trigger device will be associated with this device.
     */
    void setTrigger(IIODevice *trigger);

    /*!
     * Check if this device is a trigger device.
     */
    bool isTrigger(void);

    /*!
     * Set the number of kernel buffers to allocate to this device.
     */
    void setKernelBuffersCount(unsigned int nb_buffers);

    /*!
     * Create an IIO buffer associated with this device.
     */
    IIOBuffer createBuffer(size_t samples_count, bool cyclic);
};

/*!
 * IIOBuffer represents an IIO buffer, suitable for reading or writing samples
 * to or from the owning device.
 */
class IIOBuffer {
    friend class IIODevice;
    friend class IIOChannel;
private:
    std::shared_ptr<IIOContextRaw> ctx;
    struct iio_buffer *buffer;

    IIOBuffer(std::shared_ptr<IIOContextRaw> ctx, IIODevice *device, size_t samples_count, bool cyclic);

public:
    IIOBuffer(IIOBuffer&&);
    ~IIOBuffer(void);

    /*!
     * Get the device that this buffer belongs to.
     */
    IIODevice device(void);

    /*!
     * Set the blocking I/O mode.
     */
    void setBlockingMode(bool blocking);

    /*!
     * Get a file descriptor that can be blocked on via the poll syscall.
     */
    int fd(void);

    /*!
     * Fill the buffer with fresh samples from the owning device.
     *
     * Note that this function is only valid for buffers containing input
     * channels.
     */
    size_t refill(void);

    /*!
     * Push the buffer to the owning device.
     *
     * Note that this function is only valid for buffers containing output
     * channels.
     */
    size_t push(size_t samples_count);

    /*!
     * Get the start address of the buffer.
     */
    void* start(void);

    /*!
     * Get the address that follows the last sample in the buffer.
     */
    void* end(void);

    /*!
     * Get the step size between two samples of one channel.
     */
    ptrdiff_t step(void);
};

/*!
 * IIOChannel represents an IIO device channel exposed via libiio.
 */
class IIOChannel {
    friend class IIOAttr<IIOChannel>;
    friend class IIOAttrs<IIOChannel>;
    friend class IIODevice;
private:
    std::shared_ptr<IIOContextRaw> ctx;
    struct iio_channel *channel;

    IIOChannel(std::shared_ptr<IIOContextRaw> ctx, struct iio_channel *channel);

    const char * iio_get_attr(unsigned int idx) const;
    unsigned int iio_get_attrs_count() const;
    ssize_t iio_attr_read(const char *attr, char *dst, size_t len) const;
    ssize_t iio_attr_write(const char *attr, const char *src) const;
public:

    bool operator==(IIOChannel other) const
    {
        return this->channel == other.channel;
    }

    bool operator!=(IIOChannel other) const
    {
        return !(*this == other);
    }

    /*!
     * Get the device that this channel belongs to.
     */
    IIODevice device(void);

    /*!
     * Get the ID of this IIO device channel.
     */
    std::string id(void);

    /*!
     * Get the name of this IIO device channel, or "<unnamed>" if the channel has no name.
     */
    std::string name(void);

    /*!
     * The attributes() method returns an object exposing attributes available to
     * be read or set on this IIO channel.
     */
    IIOAttrs<IIOChannel> attributes(void);

    /*!
     * Enable this channel.
     */
    void enable(void);

    /*!
     * Disable this channel.
     */
    void disable(void);

    /*!
     * Check if this channel is enabled.
     */
    bool isEnabled(void);

    /*!
     * Check if this channel is an output channel.
     */
    bool isOutput(void);

    /*!
     * Check if this channel is a scan element.
     *
     * Scan elements are channels which can be read from or written to via the
     * IIOBuffer object.
     */
    bool isScanElement(void);

    /*!
     * Read samples belonging to this channel from an IIOBuffer.
     *
     * This function returns the number of bytes read from the buffer.
     */
    size_t read(IIOBuffer &buffer, void *dst, size_t sample_count);

    /*!
     * Write samples belonging to this channel to an IIOBuffer.
     *
     * This function returns the number of bytes written to the buffer.
     */
    size_t write(IIOBuffer &buffer, void *dst, size_t sample_count);

    /*!
     * Get the DType of this channel.
     */
    Pothos::DType dtype(void);
};

