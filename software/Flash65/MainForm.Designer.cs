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
			this.progressBar1 = new System.Windows.Forms.ProgressBar();
			this.groupBox2 = new System.Windows.Forms.GroupBox();
			this.m_txtActivity = new System.Windows.Forms.TextBox();
			this.groupBox1.SuspendLayout();
			this.groupBox2.SuspendLayout();
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
			// 
			// m_btnRead
			// 
			this.m_btnRead.Location = new System.Drawing.Point(191, 36);
			this.m_btnRead.Name = "m_btnRead";
			this.m_btnRead.Size = new System.Drawing.Size(70, 24);
			this.m_btnRead.TabIndex = 2;
			this.m_btnRead.Text = "Read";
			this.m_btnRead.UseVisualStyleBackColor = true;
			// 
			// m_btnCancel
			// 
			this.m_btnCancel.Location = new System.Drawing.Point(191, 64);
			this.m_btnCancel.Name = "m_btnCancel";
			this.m_btnCancel.Size = new System.Drawing.Size(70, 24);
			this.m_btnCancel.TabIndex = 3;
			this.m_btnCancel.Text = "Cancel";
			this.m_btnCancel.UseVisualStyleBackColor = true;
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
			// progressBar1
			// 
			this.progressBar1.Location = new System.Drawing.Point(8, 95);
			this.progressBar1.Name = "progressBar1";
			this.progressBar1.Size = new System.Drawing.Size(253, 23);
			this.progressBar1.TabIndex = 5;
			// 
			// groupBox2
			// 
			this.groupBox2.Controls.Add(this.m_txtActivity);
			this.groupBox2.Location = new System.Drawing.Point(8, 125);
			this.groupBox2.Name = "groupBox2";
			this.groupBox2.Size = new System.Drawing.Size(253, 167);
			this.groupBox2.TabIndex = 6;
			this.groupBox2.TabStop = false;
			this.groupBox2.Text = "Activity Log";
			// 
			// m_txtActivity
			// 
			this.m_txtActivity.Location = new System.Drawing.Point(9, 19);
			this.m_txtActivity.Multiline = true;
			this.m_txtActivity.Name = "m_txtActivity";
			this.m_txtActivity.ReadOnly = true;
			this.m_txtActivity.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
			this.m_txtActivity.Size = new System.Drawing.Size(238, 142);
			this.m_txtActivity.TabIndex = 0;
			// 
			// MainForm
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(269, 298);
			this.Controls.Add(this.groupBox2);
			this.Controls.Add(this.progressBar1);
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
			this.groupBox2.ResumeLayout(false);
			this.groupBox2.PerformLayout();
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
		private System.Windows.Forms.ProgressBar progressBar1;
		private System.Windows.Forms.GroupBox groupBox2;
		private System.Windows.Forms.TextBox m_txtActivity;
    }
}

