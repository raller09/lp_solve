unit dlgXLIParameters;

interface

uses
  {$IFNDEF LCL} Windows, Messages, {$ELSE} LclIntf, LMessages, LclType, LResources, {$ENDIF}
  SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls;

type
  TXLIParametersForm = class(TForm)
    Button1: TButton;
    Button2: TButton;
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  XLIParametersForm: TXLIParametersForm;

implementation

{$IFNDEF LCL}
{$R *.dfm}
{$ENDIF}

initialization
{$IFDEF LCL}
{$I dlgXLIParameters.lrs}  {Include form's resource file}
{$ENDIF}

end.
