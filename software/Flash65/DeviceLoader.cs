using System;
using System.Threading;
using System.Threading.Tasks;

namespace Flash65
{
  public enum ProgressState
  {
    Read,   // Read a page
    Write,  // Wrote a page
    Verify, // Verified a page
    Error,  // Intermediate (non-fatal) error
  }

  public enum ConnectionState
  {
    Connecting,   // Device is connecting
    Connected,    // Device is connected
    Disconnected, // Device is not connected
  }

  /// <summary>
  /// This class wraps all interaction with the remote machine in a background
  /// thread.
  /// </summary>
  public class DeviceLoader
  {
    #region "Events"
    public delegate void ProgressHandler(DeviceLoader sender, ProgressState state, int position, int target, string message);
    public event ProgressHandler Progress;

    public delegate void ConnectionStateChangedHandler(DeviceLoader sender, ConnectionState state);
    public event ConnectionStateChangedHandler ConnectionStateChanged;
    #endregion

    #region "Properties"
    public ConnectionState ConnectionState
    {
      get;
      private set;
    }

    public bool Busy
    {
      get;
      private set;
    }
    #endregion

    #region "Instance Variables"
    private string m_port;     // Name of the port to use
    private bool m_disconnect; // Request a disconnect
    #endregion

    #region "Event Dispatch"
    private void FireProgress(ProgressState state, int position, int target, string message = null)
    {
      ProgressHandler handler = Progress;
      if (handler != null)
        handler(this, state, position, target, message);
    }

    private void FireConnectionStateChanged(ConnectionState state)
    {
      ConnectionStateChangedHandler handler = ConnectionStateChanged;
      if (handler != null)
        handler(this, state);
    }
    #endregion

    #region "Implementation"
    private void Run()
    {
      System.Console.WriteLine("DeviceLoader: Starting background thread.");
      // Start the connection process
      ConnectionState = ConnectionState.Connecting;
      FireConnectionStateChanged(ConnectionState);
      // TODO: Set up serial port
      // Switch to connected state
      ConnectionState = ConnectionState.Connected;
      FireConnectionStateChanged(ConnectionState);
      // Go into a loop processing commands
      try
      {
        while (!m_disconnect)
        {
          // TODO: Check for a new command
        }
      }
      catch (Exception ex)
      {
        // TODO: Fire an error
      }
      finally
      {
        // TODO: Close down the serial port
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
      ConnectionState = ConnectionState.Disconnected;
    }

    public void Connect(string port)
    {
      if (ConnectionState != ConnectionState.Disconnected)
        throw new InvalidOperationException("Connection is already established.");
      // Store the port name and start the background thread
      m_port = port;
      Task.Factory.StartNew(() => { Run(); });
    }

    public void Disconnect()
    {
      if (ConnectionState != ConnectionState.Connected)
        throw new InvalidOperationException("Connection has not been established.");
      // Flag for disconnection
      m_disconnect = true;
    }
    #endregion
  }
}
