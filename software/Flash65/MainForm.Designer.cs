namespace Flash65
{
    partial class MainForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
			this.m_btnWrite = new System.Windows.Forms.Button();
			this.m_btnRead = new System.Windows.Forms.Button();
			this.m_btnCancel = new System.Windows.Forms.Button();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.m_btnConnect = new System.Windows.Forms.Button();
			this.m_ctlPorts = new System.Windows.Forms.ComboBox();
			this.label1 = new System.Windows.Forms.Label();
			this.m_ctlProgress = new System.Windows.Forms.ProgressBar();
			this.m_lblStatus = new System.Windows.Forms.Label();
			this.groupBox1.SuspendLayout();
			this.SuspendLayout();
			// 
			// m_btnWrite
			// 
			this.m_btnWrite.Location = new System.Drawing.Point(191, 6);
			this.m_btnWrite.Name = "m_btnWrite";
			this.m_btnWrite.Size = new System.Drawing.Size(70, 24);
			this.m_btnWrite.TabIndex = 1;
			this.m_btnWrite.Text = "Write";
			this.m_btnWrite.UseVisualStyleBackColor = true;
			this.m_btnWrite.Click += new System.EventHandler(this.OnWriteClick);
			// 
			// m_btnRead
			// 
			this.m_btnRead.Location = new System.Drawing.Point(191, 36);
			this.m_btnRead.Name = "m_btnRead";
			this.m_btnRead.Size = new System.Drawing.Size(70, 24);
			this.m_btnRead.TabIndex = 2;
			this.m_btnRead.Text = "Read";
			this.m_btnRead.UseVisualStyleBackColor = true;
			this.m_btnRead.Click += new System.EventHandler(this.OnReadClick);
			// 
			// m_btnCancel
			// 
			this.m_btnCancel.Location = new System.Drawing.Point(191, 64);
			this.m_btnCancel.Name = "m_btnCancel";
			this.m_btnCancel.Size = new System.Drawing.Size(70, 24);
			this.m_btnCancel.TabIndex = 3;
			this.m_btnCancel.Text = "Cancel";
			this.m_btnCancel.UseVisualStyleBackColor = true;
			this.m_btnCancel.Click += new System.EventHandler(this.OnCancelClick);
			// 
			// groupBox1
			// 
			this.groupBox1.Controls.Add(this.m_btnConnect);
			this.groupBox1.Controls.Add(this.m_ctlPorts);
			this.groupBox1.Controls.Add(this.label1);
			this.groupBox1.Location = new System.Drawing.Point(8, 6);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(177, 82);
			this.groupBox1.TabIndex = 4;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Communications";
			// 
			// m_btnConnect
			// 
			this.m_btnConnect.Location = new System.Drawing.Point(98, 49);
			this.m_btnConnect.Name = "m_btnConnect";
			this.m_btnConnect.Size = new System.Drawing.Size(70, 24);
			this.m_btnConnect.TabIndex = 2;
			this.m_btnConnect.Text = "Connect";
			this.m_btnConnect.UseVisualStyleBackColor = true;
			this.m_btnConnect.Click += new System.EventHandler(this.OnConnectClick);
			// 
			// m_ctlPorts
			// 
			this.m_ctlPorts.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.m_ctlPorts.FormattingEnabled = true;
			this.m_ctlPorts.Location = new System.Drawing.Point(47, 22);
			this.m_ctlPorts.Name = "m_ctlPorts";
			this.m_ctlPorts.Size = new System.Drawing.Size(121, 21);
			this.m_ctlPorts.TabIndex = 1;
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Location = new System.Drawing.Point(6, 25);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(26, 13);
			this.label1.TabIndex = 0;
			this.label1.Text = "Port";
			// 
			// m_ctlProgress
			// 
			this.m_ctlProgress.Location = new System.Drawing.Point(8, 95);
			this.m_ctlProgress.Name = "m_ctlProgress";
			this.m_ctlProgress.Size = new System.Drawing.Size(253, 23);
			this.m_ctlProgress.TabIndex = 5;
			// 
			// m_lblStatus
			// 
			this.m_lblStatus.Location = new System.Drawing.Point(5, 121);
			this.m_lblStatus.Name = "m_lblStatus";
			this.m_lblStatus.Size = new System.Drawing.Size(256, 18);
			this.m_lblStatus.TabIndex = 6;
			this.m_lblStatus.Text = "m_lblStatus";
			// 
			// MainForm
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(269, 148);
			this.Controls.Add(this.m_lblStatus);
			this.Controls.Add(this.m_ctlProgress);
			this.Controls.Add(this.groupBox1);
			this.Controls.Add(this.m_btnCancel);
			this.Controls.Add(this.m_btnRead);
			this.Controls.Add(this.m_btnWrite);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
			this.MaximizeBox = false;
			this.Name = "MainForm";
			this.Text = "Flash65";
			this.groupBox1.ResumeLayout(false);
			this.groupBox1.PerformLayout();
			this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button m_btnWrite;
        private System.Windows.Forms.Button m_btnRead;
        private System.Windows.Forms.Button m_btnCancel;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Button m_btnConnect;
        private System.Windows.Forms.ComboBox m_ctlPorts;
        private System.Windows.Forms.Label label1;
		private System.Windows.Forms.ProgressBar m_ctlProgress;
		private System.Windows.Forms.Label m_lblStatus;
    }
}

