#ifndef OPERA_LOCAL_SERVER_INTERFACE_H__
#define OPERA_LOCAL_SERVER_INTERFACE_H__

// Interface used to read URL data from URLs stored in the Gears DB
//
// The cache reader is initialized by calling GetHeaderData() and then
// all the other methods will return valid data.
class OperaCacheReaderInterface {
 public:
  // Called to fetch the list of headers for a specified URL.
  // The name of the URL to fetch the headers of should be in url.
  // After this function returns, header_str will point to an array of header
  // lines.
  // The number of entries in the header array will be returned in header_length
  virtual bool GetHeaderData(const unsigned short *url,
                             char **header_str,
                             int *header_length) = 0;

  // Called to indicate that the stored headers should be deleted.
  virtual void ReleaseHeaderData() = 0;

  // Called repeatedly to fetch the actual data the URL points to.
  // After the call returns, the pre-allocated header_str buffer will be
  // filled with the next chunk of data.
  // The buffer will be filled with the first buffer_length chars of data, or
  // less if there is less data remaining for the URL.
  // The return value will indicate how many bytes were actually read. To get
  // all the data, this function must be called until More() returns false.
  virtual int GetBodyData(char *header_str, int buffer_length) = 0;

  // Returns true if there is more stored data to fetch with GetBodyData().
  virtual bool More() = 0;

  // Called when we no longer need the Cache reader and it can be deleted.
  virtual void Destroy() = 0;

 protected:
  virtual ~OperaCacheReaderInterface() {}
};

// Interface for fetching URL data from the Gears DB
//
// If CanService() returns true, a cache reader can be created to get the data.
class OperaLocalServerInterface {
 public:
  // Returns true if the URL indicated by url can be served from
  // the Gears database.
  virtual bool CanService(const unsigned short *url) = 0;

  // Returns a cache reader to read data from the DB.
  virtual OperaCacheReaderInterface *GetCacheReader() = 0;

  // Called when the local server is not needed and can be deleted.
  virtual void Destroy() = 0;

 protected:
  virtual ~OperaLocalServerInterface() {}
};

#endif // OPERA_LOCAL_SERVER_INTERFACE_H__
