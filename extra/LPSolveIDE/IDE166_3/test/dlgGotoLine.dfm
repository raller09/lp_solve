object GotoLineForm: TGotoLineForm
  Left = 628
  Top = 163
  ActiveControl = LineNumer
  BorderStyle = bsDialog
  Caption = 'Goto line number'
  ClientHeight = 94
  ClientWidth = 329
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = True
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 24
    Top = 24
    Width = 105
    Height = 13
    Caption = 'Enter new line number'
  end
  object Button1: TButton
    Left = 24
    Top = 64
    Width = 75
    Height = 25
    Caption = 'OK'
    TabOrder = 0
    OnClick = Button1Click
  end
  object Button2: TButton
    Left = 232
    Top = 64
    Width = 75
    Height = 25
    Cancel = True
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 1
  end
  object LineNumer: TEdit
    Left = 184
    Top = 24
    Width = 121
    Height = 21
    TabOrder = 2
    Text = '0'
    OnKeyPress = LineNumerKeyPress
  end
end
