function [x, e] = LeastSquareSolution(fileNameA, fileNameB, output)
    inputData = load(fileNameA);
    A = inputData;
    inputData = load(fileNameB);
    b = inputData;
    % singular value decomposition
    CI = [];
    
    %testing one sample at a time, using all remaining samples as training data

   total_folds = 10;
   fold_size = length(b)/total_folds;

   total_folds = floor(total_folds)
   remainder_of_total_folds = length(b) - total_folds*fold_size
   elements_per_fold = fold_size;
   
   solution=[];
   
   for i=0:(total_folds-1)
       
        % debug
        file_id_number = num2str((i+1),'%2d');
        
        debug_file = strcat(output, '/');
        debug_file = strcat(debug_file, 'Fold_');
        debug_file = strcat(debug_file, file_id_number); 
        debug_file = strcat(debug_file, '.txt');
        
        if (i == (total_folds-1))
            elements_per_fold = fold_size + remainder_of_total_folds
        end
        %select one sample at a time for testing using the rest for training
        %if the value is set to 1, that is the sample that will be used for
        %training/testing

        train = ones(length(b),1); %create a column vector of ones

        for k=(i*fold_size+1):((i*fold_size) + elements_per_fold)
            train(k) = 0;   %set the i-th sample to be the test sample, all others are used for training
        end

        train = ismember(train, 1); %converts to logical the train set

        test = ismember(train, 0);  %converts to logical the test set

        A_ = A(train,:); % A_ will contain all the data except the test data. This is the train data.
        b_ = b(train); % b_ will contain all status except the test status
        [m n] = size(A_);
    
        % do the SVD on the test A data
        [U,S,V] = svd(A_);
        
        % this value should be equal to A_
        U*S*V';
        

        % compute the c from the training b data
        c = U'*b_;
        
        % compute y on from the singular values
        y=[];
        for j=1:n
            yj = c(j)/S(j,j);
            y = [y; yj];
        end
 
        % compute the unknown x values we are trying to find the least
        % square approximation of
        x = V*y;
        
        % the error estimate on the training data
        e = A_*x - b_;

        % add the solution to the solution vector
        solution = [solution; x']

        % compute the ci value of the test data
        test_data = A(test,:); % this extract just a row vector
        size(test_data)
        
        ci = test_data*x;      % compute the ci value for this test sample
        
        %saving the calculated cis
        CI = [CI; ci];         % store the ci values
       
    
        %fid_debug = fopen(debug_file,'w');
        %fprintf(fid_debug, '%s\n','Train data');
        %fprintf(fid_debug, '%8.6f\t%8.6f\t%8.6f\t%8.6f\t%8.6f\t%8.6f\t%8.6f\t%8.6f', A_);
%         fprintf(fid_debug, '%s\n','Test data');
%         fprintf(fid_debug, '%8.6f\t%8.6f\t%8.6f\t%8.6f\t%8.6f\t%8.6f\t%8.6f\t%8.6f', test_data);
        
        fid_debug = fopen(debug_file,'w');
%         fprintf(fid_debug, '%s\n\n','Test data');
        
        
        fprintf(fid_debug, '%s\n\n','new CI');
        fprintf(fid_debug, '%8.6f\n', ci);
        
        fprintf(fid_debug, '%s\n\n','actual CI');
        actual_ci = b(test,:);
        fprintf(fid_debug, '%8.6f\n', actual_ci);
        fprintf(fid_debug, '%s\n\n','error');
        fprintf(fid_debug, '%8.6f\n', (ci-actual_ci));
%         fprintf(fid_debug, '%s\n\n','Train data');
        
        
        fclose(fid_debug);

        dlmwrite(debug_file, A_, 'delimiter', '\t', '-append');
        dlmwrite(debug_file, test_data, 'delimiter', '\t', '-append');

   
   end
   
   % write out the ci values
   fid_ci = fopen('newCIs.txt','w');
   fprintf(fid_ci, '%8.6f\n', CI);
   fclose(fid_ci);
   
   %fid_solution_vectors = fopen('Solution_vectors.txt', 'w');
   dlmwrite('Solution_vectors.txt', solution, 'delimiter', '\t');
   %fclose(fid_solution_vectors);
   
        
