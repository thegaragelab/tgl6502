using System;
using System.IO;
using System.IO.Ports;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Flash65
{
    public partial class MainForm : Form
    {
      private DeviceLoader m_loader;

      public MainForm()
      {
        InitializeComponent();
        // Set up the loader
        m_loader = new DeviceLoader();
        m_loader.ConnectionStateChanged += OnConnectionStateChanged;
        m_loader.Progress += OnProgress;
		m_loader.Error += OnError;
		m_loader.OperationComplete += OnOperationComplete;
        UpdateUI("");
		// Populate the list of available COM ports
		List<string> ports = new List<string>(SerialPort.GetPortNames());
		ports.Sort();
		m_ctlPorts.Items.AddRange(ports.ToArray());
		if (m_ctlPorts.Items.Count > 0)
		  m_ctlPorts.SelectedIndex = 0;
      }

	  void OnOperationComplete(DeviceLoader sender, Operation operation, bool withSuccess)
	  {
		if (InvokeRequired)
		{
		  BeginInvoke(new Action(() => { OnOperationComplete(sender, operation, withSuccess); }));
		  return;
		}
		// Safe to work with UI
		UpdateUI(String.Format("{0} complete.", operation));
		// TODO: Handle the results of the operation
		// Clean up progress state
		m_ctlProgress.Value = m_ctlProgress.Minimum;
	  }

	  void OnError(DeviceLoader sender, string message, Exception ex)
	  {
		if (InvokeRequired)
		{
		  BeginInvoke(new Action(() => { OnError(sender, message, ex); }));
		  return;
		}
		// Safe to work with UI
		// TODO: Implement this
	  }

      void OnProgress(DeviceLoader sender, ProgressState state, int position, int target, string message)
      {
        if (InvokeRequired)
        {
          BeginInvoke(new Action(() => { OnProgress(sender, state, position, target, message); }));
          return;
        }
        // Safe to work with UI
		m_ctlProgress.Maximum = target;
		m_ctlProgress.Value = position;
		UpdateUI(message);
      }

      void OnConnectionStateChanged(DeviceLoader sender, ConnectionState state)
      {
        if (InvokeRequired)
        {
          BeginInvoke(new Action(() => { OnConnectionStateChanged(sender, state); }));
          return;
        }
        // Safe to work with UI
        UpdateUI(String.Format("Device is {0}", state));
      }

      private void UpdateUI(string message = null)
      {
        // Update the connect button based on current state
        m_btnConnect.Enabled = true;
        switch (m_loader.ConnectionState)
        {
          case ConnectionState.Connected:
            m_btnConnect.Text = "Disconnect";
            break;
          case ConnectionState.Disconnected:
            m_btnConnect.Text = "Connect";
            break;
          case ConnectionState.Connecting:
            m_btnConnect.Text = "Connect";
            m_btnConnect.Enabled = false;
            break;
        }
		// Change the other buttons
        m_btnCancel.Enabled = m_loader.Busy;
        m_btnRead.Enabled = (m_loader.ConnectionState == ConnectionState.Connected) && (!m_loader.Busy);
		m_btnWrite.Enabled = (m_loader.ConnectionState == ConnectionState.Connected) && (!m_loader.Busy);
		// Add the message (if any)
		if (message == null)
		  message = "";
		m_lblStatus.Text = message;
      }

	  #region "UI Event Handlers"
	  private void OnConnectClick(object sender, EventArgs e)
      {
        try
        {
          if (m_loader.ConnectionState == ConnectionState.Disconnected)
          {
            // TODO: Verify that a serial port is connected
            m_loader.Connect("");
          }
          else
            m_loader.Disconnect();
        }
        catch (Exception ex)
        {
          System.Console.WriteLine("Error connecting - {0}", ex.ToString());
          UpdateUI("Connection attempt failed.");
        }
      }

	  private void OnWriteClick(object sender, EventArgs e)
	  {
	    // TODO: Implement this
		UpdateUI();
	  }

	  private void OnReadClick(object sender, EventArgs e)
	  {
		  m_loader.Read();
		  UpdateUI();
	  }

	  private void OnCancelClick(object sender, EventArgs e)
	  {
		  m_loader.Cancel();
		  UpdateUI();
	  }
	  #endregion
	}
}
