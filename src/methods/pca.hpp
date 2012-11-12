/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Copyright (c) 2012, Sergey Lisitsyn
 */

#ifndef TAPKEE_PCA_H_
#define TAPKEE_PCA_H_

#include "../defines.hpp"
#include "../utils/time.hpp"

template <class RandomAccessIterator, class FeatureVectorCallback>
EmbeddingResult project(const ProjectionResult& projection_result, RandomAccessIterator begin,
                        RandomAccessIterator end, FeatureVectorCallback callback, unsigned int dimension)
{
	timed_context context("Data projection");

	DenseVector current_vector(dimension);

	const DenseSymmetricMatrix& projection_matrix = projection_result.first;

	DenseMatrix embedding = DenseMatrix::Zero((end-begin),projection_matrix.cols());

	for (RandomAccessIterator iter=begin; iter!=end; ++iter)
	{
		callback(*iter,current_vector);
		embedding.row(iter-begin) = projection_matrix.transpose()*current_vector;
	}

	return EmbeddingResult(embedding,DenseVector());
}

template <class RandomAccessIterator, class FeatureVectorCallback>
DenseSymmetricMatrix compute_covariance_matrix(RandomAccessIterator begin, RandomAccessIterator end, 
                                               FeatureVectorCallback callback, unsigned int dimension)
{
	timed_context context("Constructing PCA covariance matrix");

	DenseSymmetricMatrix covariance_matrix = DenseSymmetricMatrix::Zero(dimension,dimension);
	
	DenseVector sum = DenseVector::Zero(dimension);
	DenseVector current_vector(dimension);
	for (RandomAccessIterator iter=begin; iter!=end; ++iter)
	{
		callback(*iter,current_vector);
		sum += current_vector;
		covariance_matrix.selfadjointView<Eigen::Upper>().rankUpdate(current_vector,1.0);
	}
	covariance_matrix.selfadjointView<Eigen::Upper>().rankUpdate(sum,-1.0/(end-begin));

	return covariance_matrix;
};

template <class RandomAccessIterator, class KernelCallback>
DenseSymmetricMatrix compute_centered_kernel_matrix(RandomAccessIterator begin, RandomAccessIterator end, 
                                                    KernelCallback callback)
{
	timed_context context("Constructing kPCA centered kernel matrix");

	DenseSymmetricMatrix kernel_matrix(end-begin,end-begin);

	for (RandomAccessIterator i_iter=begin; i_iter!=end; ++i_iter)
	{
		for (RandomAccessIterator j_iter=i_iter; j_iter!=end; ++j_iter)
		{
			DefaultScalarType k = callback(*i_iter,*j_iter);
			kernel_matrix(i_iter-begin,j_iter-begin) = k;
			kernel_matrix(j_iter-begin,i_iter-begin) = k;
		}
	}

	DenseVector col_means = kernel_matrix.colwise().mean();
	DefaultScalarType grand_mean = kernel_matrix.mean();
	kernel_matrix.array() += grand_mean;
	kernel_matrix.colwise() -= col_means;
	kernel_matrix.rowwise() -= col_means.transpose();

	return kernel_matrix;
};

#endif
