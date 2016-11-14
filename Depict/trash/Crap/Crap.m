function Crap(varargin)

global The_files_to_cluster
global The_mask

global connectedcut
global interactive
global vol_begin
global vol_end
global winlen
global NCUT
global SPATIALCUT
global RHO
global NCLUST_MAX



The_files_to_cluster=[]
The_mask=[]

global defaults    %%% def. spm parameters
global st



    try
        if numel(defaults)==0
            try
                spm_defaults;
            catch
                try
                    spm('ChMod','FMRI')
                end
            end
        end
        defaults.oldDefaults = defaults;
    end



fg = crap_figure('GetWin','Graphics1');


if isempty(fg), error('Can''t create graphics window'); end
crap_figure('Clear','Graphics1');

set(gcf,'DefaultUicontrolFontSize',spm('FontSizes',get(gcf,'DefaultUicontrolFontSize')));

WS = spm('WinScale');

    st.SPM = spm('FnBanner');

uicontrol(fg,'Style','Text','Position',[25 680 550 55].*WS,'String','SPM CRAP TOOLBOX  v1.0',...
    'FontSize',30,'FontWeight','bold','BackgroundColor',[1 1 1],'HorizontalAlignment','Center');


uicontrol(fg,'Style','PushButton','Units','normalized','Position',[.05 .70 .9 .055],'Callback','crap_select_input_files;',...
    'String','Crap select input files','FontSize',spm('FontSizes',12));

uicontrol(fg,'Style','PushButton','Units','normalized','Position',[.05 .70-2*0.075 .9 .055],'Callback','crap_select_mask;',...
    'String','Crap select mask','FontSize',spm('FontSizes',12));

uicontrol(fg,'Style','PushButton','Units','normalized','Position',[.05 .70-4*0.075 .9 0.055],'Callback','crap_select_parameters;',...
    'String','Crap select options','FontSize',spm('FontSizes',12));

uicontrol(fg,'Style','PushButton','Units','normalized','Position',[.05 .70-6*0.075 .9 0.055],'Callback','crap_apply;',...
    'String','Crap apply','FontSize',spm('FontSizes',12));


uicontrol(fg,'Style','PushButton','Units','normalized','Position',[.9 .01 .05 .03],'Callback','crap_exit;',...
    'String',{'Quit'},'FontSize',spm('FontSizes',10));
























