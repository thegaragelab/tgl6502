using System;
using System.IO;
using System.IO.Ports;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Collections.Generic;

namespace Flash65
{
  /// <summary>
  /// State information for progress events.
  /// </summary>
  public enum ProgressState
  {
    Read,   // Read a page
    Write,  // Wrote a page
    Verify, // Verified a page
    Error,  // Intermediate (non-fatal) error
  }

  /// <summary>
  /// The possible connection states.
  /// </summary>
  public enum ConnectionState
  {
    Connecting,   // Device is connecting
    Connected,    // Device is connected
    Disconnected, // Device is not connected
  }

  /// <summary>
  /// The possible operations the device can be performing.
  /// </summary>
  public enum Operation
  {
    Idle,    // No operation is being performed
    Reading, // Reading flash contents
    Writing, // Writing flash contents
  }

  /// <summary>
  /// This class wraps all interaction with the remote machine in a background
  /// thread.
  /// </summary>
  public class DeviceLoader
  {
    #region "Constants"
    /// <summary>
    /// Total size of the EEPROM
    /// </summary>
    public const UInt32 EEPROM_SIZE = 128 * 1024;

    /// <summary>
    /// Size of individual pages in the EEPROM.
    /// </summary>
    public const UInt16 EEPROM_PAGE = 128;

    /// <summary>
    /// Maximum number of retries for failed operations
    /// </summary>
    public const int MAX_RETRIES = 3;

    /// <summary>
    /// The baud rate to use.
    /// </summary>
    private const int BAUD_RATE = 57600;

    /// <summary>
    /// Identifier string used to detect the device
    /// </summary>
    private const string IDENT_STRING = "TGL-6502 ";

    /// <summary>
    /// Character to send to request EEPROM mode
    /// </summary>
    private const byte MODE_REQUEST_CHAR = 0x55;

    /// <summary>
    /// Character sent to indicate success
    /// </summary>
    private const byte OPERATION_SUCCESS = 0x2B; // '+'

    /// <summary>
    /// Character sent to indicate failure
    /// </summary>
    private const byte OPERATION_FAILED = 0x2D; // '-'

    /// <summary>
    /// Command code to start reading
    /// </summary>
    private const byte COMMAND_READ = 0x52; // 'R'

    /// <summary>
    /// Command code to start writing
    /// </summary>
    private const byte COMMAND_WRITE = 0x57; // 'W'
    #endregion

    #region "Events"
    public delegate void ProgressHandler(DeviceLoader sender, ProgressState state, int position, int target, string message);
    public event ProgressHandler Progress;

    public delegate void OperationCompleteHandler(DeviceLoader sender, Operation operation, bool withSuccess);
    public event OperationCompleteHandler OperationComplete;

    public delegate void ConnectionStateChangedHandler(DeviceLoader sender, ConnectionState state);
    public event ConnectionStateChangedHandler ConnectionStateChanged;

    public delegate void ErrorHandler(DeviceLoader sender, string message, Exception ex);
    public event ErrorHandler Error;
    #endregion

    #region "Properties"
    /// <summary>
    /// The current connection state.
    /// </summary>
    public ConnectionState ConnectionState
    {
      get;
      private set;
    }

    /// <summary>
    /// The active operation. This property is only valid when the device is
    /// in the 'Connected' state.
    /// </summary>
    public Operation Operation
    {
      get;
      private set;
    }

    /// <summary>
    /// Indicates whether the loader is busy or not.
    /// </summary>
    public bool Busy
    {
      get;
      private set;
    }

    /// <summary>
    /// Provide access to the data read or written to device.
    /// </summary>
    public byte[] Data
    {
      get;
      private set;
    }
    #endregion

    #region "Instance Variables"
    private string         m_port;       // Name of the port to use
    private bool           m_disconnect; // Request a disconnect
    private bool           m_cancel;     // Cancel of the current operation
    private AutoResetEvent m_event;      // Event to control command queue
    private SerialPort     m_serial;     // The serial port for communication
    private Random         m_random;     // For debugging only 
    private byte[]         m_scratch; // Scratchpad buffer
    #endregion

    #region "Event Dispatch"
    private void FireError(string message, Exception ex = null)
    {
      ErrorHandler handler = Error;
      if (handler != null)
	handler(this, message, ex);
    }

    private void FireProgress(ProgressState state, int position, int target, string message = null)
    {
      ProgressHandler handler = Progress;
      if (handler != null)
        handler(this, state, position, target, message);
    }

    private void FireOperationComplete(Operation operation, bool withSuccess)
    {
      OperationCompleteHandler handler = OperationComplete;
      if (handler != null)
	handler(this, operation, withSuccess);
    }

    private void FireConnectionStateChanged(ConnectionState state)
    {
      ConnectionStateChangedHandler handler = ConnectionStateChanged;
      if (handler != null)
        handler(this, state);
    }
    #endregion

    #region "Implementation"
    private byte ReadByte()
    {
      byte result = (byte)m_serial.ReadByte();
      System.Console.WriteLine("< 0x{0:X2}", result);
      return result;
    }

    private void WriteByte(byte value)
    {
      m_scratch[0] = value;
      m_serial.Write(m_scratch, 0, 1);
      System.Console.WriteLine("> 0x{0:X2}", value);
    }

    /// <summary>
    /// Read a page from the device EEPROM.
    /// </summary>
    /// <param name="page">The page number to read.</param>
    /// <param name="buffer">The buffer to store the data.</param>
    /// <param name="offset">Offset into the buffer to start writing.</param>
    /// <returns>True if the device acknowledged success</returns>
    private bool ReadPage(UInt16 page, byte[] buffer, UInt32 offset)
    {
      System.Console.WriteLine("Reading page {0}", page);
      // Send the command
      WriteByte(COMMAND_READ);
      WriteByte((byte)(page >> 8));
      WriteByte((byte)(page & 0xFF));
      // Check the response
      if (ReadByte() != OPERATION_SUCCESS)
        return false;
      // Read the data and calculate the checksum
      UInt16 actual = 0;
      for (int i = 0; i < EEPROM_PAGE; i++)
      {
        buffer[offset + i] = ReadByte();
        actual += (UInt16)buffer[offset + i];
      }
      // Read the checksum
      UInt16 checksum = (UInt16)(((UInt16)ReadByte()) << 8);
      checksum |= (UInt16)ReadByte();
      // And verify it
      return actual == checksum;
    }

    /// <summary>
    /// Write a page to the device EEPROM
    /// </summary>
    /// <param name="page">The page number to write</param>
    /// <param name="buffer">The buffer containing the data to write.</param>
    /// <param name="offset">Offset into the buffer to start writing from.</param>
    /// <returns>True if the device acknowledged success</returns>
    private bool WritePage(UInt16 page, byte[] buffer, UInt32 offset)
    {
      System.Console.WriteLine("Writing page {0}", page);
      // Send the command
      WriteByte(COMMAND_WRITE);
      WriteByte((byte)(page >> 8));
      WriteByte((byte)(page & 0xFF));
      // Write the data and calculate the checksum
      UInt16 actual = 0;
      for (int i = 0; i < EEPROM_PAGE; i++)
      {
        WriteByte(buffer[offset + i]);
        actual += (UInt16)buffer[offset + i];
      }
      // Write the checksum
      WriteByte((byte)(actual >> 8));
      WriteByte((byte)(actual & 0xFF));
      // Check the response
      return ReadByte() == OPERATION_SUCCESS;
    }

    /// <summary>
    /// This implements the reading loop. It will continue until the operation
    /// is complete, is canceled or the device is disconnected.
    /// </summary>
    private void ReadData()
    {
      // Set state
      Busy = true;
      UInt32 offset = 0;
      UInt16 page = 0;
      int retries = 0;
      System.Console.WriteLine("Reading EEPROM ...");
      while ((!m_cancel) && (!m_disconnect) && (offset < EEPROM_SIZE))
      {
	// Read the current page
	FireProgress(ProgressState.Read, (int)offset, (int)EEPROM_SIZE, String.Format("Reading page {0}", page));
	if (!ReadPage(page, Data, offset))
	{
	  retries++;
	  if (retries>MAX_RETRIES) 
	  {
	    FireError(String.Format("Read operation failed after {0} retries.", MAX_RETRIES));
	    break;
	  }
	  else 
	  {
	    FireProgress(ProgressState.Error, (int)offset, (int)EEPROM_SIZE, "Read failed, retrying");
	  }
	}
	else
	{
	  retries = 0;
	  offset += EEPROM_PAGE;
	  page++;
	}
      }
      // Signal the end of the operation
      Busy = false;
      Operation = Operation.Idle;
      FireOperationComplete(Operation.Reading, retries == 0);
    }

    /// <summary>
    /// This implements the writing loop. It will continue until the operation
    /// is complete, is canceled or the device is disconnected.
    /// </summary>
    private void WriteData()
    {
      // Set state
      Busy = true;
      UInt32 offset = 0;
      UInt16 page = 0;
      int write_retries = 0;
      int read_retries = 0;
      int verify_retries = 0;
      bool verified = true;
      byte[] buffer = new byte[EEPROM_PAGE];
      while ((!m_cancel) && (!m_disconnect) && (offset < Data.Length))
      {
	// Read the current page
	FireProgress(ProgressState.Write, (int)offset, Data.Length, String.Format("Writing page {0}", page));
	if (!WritePage(page, Data, offset))
	{
	  System.Console.WriteLine("Write failed.");
	  write_retries++;
	  if (write_retries > MAX_RETRIES)
	  {
	    FireError(String.Format("Write operation failed after {0} retries.", MAX_RETRIES));
	    break;
	  }
	  else
	  {
	    FireProgress(ProgressState.Error, (int)offset, Data.Length, "Write failed, retrying");
	  }
	}
	else
	{
	  // Verify the page
	  read_retries = 0;
	  while (read_retries <= MAX_RETRIES)
	  {
	    FireProgress(ProgressState.Verify, (int)offset, Data.Length, String.Format("Verifying page {0}", page));
	    if (ReadPage(page, buffer, 0))
	    {
              verified = true;
              for (int i = 0; i < EEPROM_PAGE; i++)
                verified = verified & (buffer[i] == Data[offset + i]);
	      if (!verified)
		System.Console.WriteLine("Verify failed.");
	      break;
	    }
	    // Try again
	    System.Console.WriteLine("Read failed.");
	    read_retries++;
	  }
	  if (read_retries > MAX_RETRIES)
	  {
	    FireError(String.Format("Read operation failed after {0} retries.", MAX_RETRIES));
	    break;
	  }
	  if (verify_retries > MAX_RETRIES)
	  {
	    FireError(String.Format("Verification failed after {0} retries.", MAX_RETRIES));
	    break;
	  }
	  if (!verified) 
	  {
	    FireProgress(ProgressState.Error, (int)offset, Data.Length, "Verification failed, retrying");
	    verify_retries++;
	  }
	  else
	  {
	    read_retries = 0;
	    write_retries = 0;
	    verify_retries = 0;
	    offset += EEPROM_PAGE;
	    page++;
	  }
	}
      }
      // Signal the end of the operation
      Busy = false;
      Operation = Operation.Idle;
      FireOperationComplete(Operation.Writing, (read_retries + write_retries + verify_retries) == 0);
    }

    private void CloseOnError(string message, Exception ex = null)
    {
      m_serial.Close();
      m_serial = null;
      FireError(message, ex);
      ConnectionState = ConnectionState.Disconnected;
      FireConnectionStateChanged(ConnectionState);
    }

    /// <summary>
    /// Main thread loop.
    /// </summary>
    private void Run()
    {
      System.Console.WriteLine("DeviceLoader: Starting background thread.");
      // Start the connection process
      ConnectionState = ConnectionState.Connecting;
      FireConnectionStateChanged(ConnectionState);
      // Set up serial port
      m_serial = new SerialPort(m_port, BAUD_RATE, Parity.None, 8, StopBits.One);
      m_serial.ReadTimeout = 10000;
      m_serial.Open();
      try
      {
        // Wait for and verify the identity string
        while (ReadByte() != '\n') ;
        List<byte> ident = new List<byte>();
        for (byte ch = ReadByte(); (ch != '\n') && (ident.Count < 128); ch = ReadByte())
          ident.Add(ch);
        ASCIIEncoding encoding = new ASCIIEncoding();
        string decoded = encoding.GetString(ident.ToArray(), 0, ident.Count);
        if (!decoded.StartsWith(IDENT_STRING))
        {
          CloseOnError("Failed to received identification string.");
          return;
        }
        // Enter EEPROM mode
        WriteByte(MODE_REQUEST_CHAR);
        // Wait for the OK response
        if (ReadByte() != OPERATION_SUCCESS)
        {
          CloseOnError("Device did not enter loading mode.");
          return;
        }
      }
      catch (Exception ex)
      {
        CloseOnError("Failed to establish connection.", ex);
        return;
      }
      // Switch to connected state
      ConnectionState = ConnectionState.Connected;
      FireConnectionStateChanged(ConnectionState);
      // Go into a loop processing commands
      try
      {
	// Wait for a command
        while (!m_disconnect)
        {
	  m_event.WaitOne(250);
	  switch (Operation)
	  {
	    case Operation.Idle:
	      // Do nothing
	      break;
	    case Operation.Reading:
	      ReadData();
	      break;
	    case Operation.Writing:
	      WriteData();
	      break;
	  }
        }
      }
      catch (Exception ex)
      {
	FireError("Unhandled exception in communications thread.", ex);
      }
      finally
      {
        // Close down the serial port
        m_serial.Close();
        m_serial = null;
        // Change the connection state
        ConnectionState = ConnectionState.Disconnected;
        FireConnectionStateChanged(ConnectionState);
        m_disconnect = false;
      }
      System.Console.WriteLine("DeviceLoader: Ending background thread.");
    }
    #endregion

    #region "Public Methods"
    public DeviceLoader()
    {
      m_event = new AutoResetEvent(false);
      ConnectionState = ConnectionState.Disconnected;
      Operation = Operation.Idle;
      m_random = new Random();
    }

    public void Connect(string port)
    {
      if (ConnectionState != ConnectionState.Disconnected)
        throw new InvalidOperationException("Connection is already established.");
      // Store the port name and start the background thread
      m_port = port;
      m_scratch = new byte[1];
      Task.Factory.StartNew(() => { Run(); });
    }

    public void Disconnect()
    {
      if (ConnectionState != ConnectionState.Connected)
        throw new InvalidOperationException("Connection has not been established.");
      // Flag for disconnection
      m_disconnect = true;
    }

    /// <summary>
    /// Write a block of data to the EEPROM. This is an asynchronous operation,
    /// monitor the progress events to determine when it is complete.
    /// </summary>
    /// <param name="data">
    ///   The data to write to the EEPROM, must be non-null and contain at least
    ///   one byte.
    /// </param>
    public void Write(byte[] data)
    {
      // Make sure we can start an operation
      if ((ConnectionState != ConnectionState.Connected) || (Operation != Operation.Idle))
	throw new InvalidOperationException("An operation is already active or the device is not connected.");
      // Make sure we have some data
      if ((data == null) || (data.Length == 0))
	throw new ArgumentException("Write operation requires data to be provided.");
      // Set up our state
      Data = data;
      Operation = Operation.Writing;
      m_event.Set();
    }

    /// <summary>
    /// Read the entire contents of the EEPROM into the data buffer. This is an
    /// asynchronous operation - monitor the progress events to determine when
    /// it is complete.
    /// </summary>
    public void Read()
    {
      // Make sure we can start an operation
      if ((ConnectionState != ConnectionState.Connected) || (Operation != Operation.Idle))
	throw new InvalidOperationException("An operation is already active or the device is not connected.");
      // Set up the data buffer
      Data = new byte[EEPROM_SIZE];
      // Set up our state
      Operation = Operation.Reading;
      m_event.Set();
    }

    /// <summary>
    /// Cancel the current operation.
    /// </summary>
    public void Cancel()
    {
      // Make sure we are connected
      if (ConnectionState != ConnectionState.Connected)
	throw new InvalidOperationException("Device is not connected.");
      // Only cancel if an operation is in progress
      if (Operation != Operation.Idle)
	m_cancel = true;
    }
    #endregion
  }
}
