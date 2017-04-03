function depict_generate_output_nifti(outfname,The_files_to_cluster,The_mask,final_assignation,density,vol,winlen)

global winlen

    true_NCLUST=max(final_assignation);
    dmax=max(density);

   if(~isempty(The_mask))
     brain = spm_read_vols(The_mask);
     brain=permute(brain,[2 1 3]);
   else
     brain = spm_read_vols(The_files_to_cluster(1));
     brain(brain <=100)=0; 
     brain = permute(brain,[2 1 3]);
   end

   dim = The_files_to_cluster(1).dim;

   out_data=zeros(dim(1),dim(2),dim(3),true_NCLUST);

   count=0;

    for i=1:dim(1)
      for j=1:dim(2)
        for k=1:dim(3)
          if(brain(i,j,k)>0 )
          count=count+1;
            if(final_assignation(count)>0)
              density(count)/dmax*100;
              out_data(i,j,k,final_assignation(count))=density(count)/dmax*100;
            end
          end
        end
      end
    end

   for cl=1:true_NCLUST
       if(length(find(final_assignation==cl)) > 50)	    
	    output(cl)=The_files_to_cluster(1);
       	    outfname1=[outfname '_' num2str(vol) '_' num2str(vol+winlen-1) '_' num2str(cl) '.nii'];
            output(cl).fname = outfname1;
            Image = spm_create_vol(output(cl));
            Image=spm_write_vol(output(cl), out_data(:,:,:,cl));
	end
    end
    clear matlabbatch;
    [path, name, ext] = fileparts(outfname);
    name1=[name  '_' num2str(vol) '_' num2str(vol+winlen-1) '_.*.nii$'];
    temp = cellstr(spm_select('FPList', path, name1));

    matlabbatch{1}.spm.util.cat.vols = temp;
    matlabbatch{1}.spm.util.cat.name = [outfname '_map_' num2str(vol) '_' num2str(vol+winlen-1) '.nii'];
    matlabbatch{1}.spm.util.cat.dtype = 0;
    spm_jobman('initcfg');
    spm_jobman('run',matlabbatch);

    outfname1=[outfname '_'  num2str(vol) '_' num2str(vol+winlen-1) '_*'];
    delete(outfname1);
    outfname1=[outfname '_map_'  num2str(vol) '_' num2str(vol+winlen-1) '*.mat'];
    delete(outfname1);


end

