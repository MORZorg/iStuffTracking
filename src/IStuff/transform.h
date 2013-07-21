#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <cmath>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"

using namespace std;
using namespace cv;

Mat transformation( Mat, Mat, Size );
Mat rotation( Mat, float );
Mat scale( Mat, float, float );
int* nearest_neighbour( float*, Size);
float* to_euclid( float, float, Size );
float* to_matrix( float, float, Size );

// Funzione che opera il prodotto matrice vettore su ogni punto della matrice image per effettuare la trasformazione. La matrice di trasformazione deve essere fornita in ingresso come parametro di tipo Mat
Mat transformation( Mat image, Mat transformationMatrix, Size transformedSize ) {
	Size imageSize = image.size();

	Mat transformedImage = Mat( transformedSize, CV_8UC3, Scalar( 122, 122, 122 ) );

	for( int i = 0; i < transformedSize.height; i++ ) {
		for( int j = 0; j < transformedSize.width; j++ ) {
			float* euclidTransformed = to_euclid( i, j, transformedSize );
			Mat euclidTransformedMat = ( Mat_<float>( 2, 1 ) << euclidTransformed[0], euclidTransformed[1] );

			Mat euclidSource = transformationMatrix * euclidTransformedMat;

			float* matrixOriginal = to_matrix( euclidSource.at<float>( 0, 0 ), euclidSource.at<float>( 1, 0 ), imageSize );

			int* matrixProper = nearest_neighbour( matrixOriginal, imageSize );

			// Se il punto è fuori dai limiti dell'immagine lo lascio grigio
			if( matrixProper != NULL )
				transformedImage.at<Vec3b>( i, j ) = image.at<Vec3b>( matrixProper[0], matrixProper[1] );
		}
	}

	return transformedImage;
}

// Funzione di rotazione dell'immagine di un angolo specificato attorno al proprio centro. L'angolo deve essere in radianti. La dimensione della nuova immagine è calcolata considerando il modulo dell'angolo di rotazione per evitare dimensioni negative (tanto un'immagine ruotata di + o - alfa assume la stessa dimensione)
Mat rotation( Mat image, float angle ) {
	Size imageSize = image.size();
	Size rotatedSize( imageSize.width * cos( abs( angle ) ) + imageSize.height * sin( abs( angle ) ), imageSize.width * sin( abs( angle ) ) + imageSize.height * cos( abs( angle ) ) );
	//Size rotatedSize( max( imageSize.width, imageSize.height ), max( imageSize.width, imageSize.height ) );

	Mat rotationMatrix = ( Mat_<float>( 2, 2 ) << cos( angle ), -sin( angle ), sin( angle ), cos( angle ) );
	Mat inverseRotation = rotationMatrix.inv();

	return transformation( image, inverseRotation, rotatedSize );
}

// Funzione di scalatura dell'immagine tramite un certo ratio differenziabile sulle due dimensioni, larghezza w e altezza h
Mat scale( Mat image, float w_ratio, float h_ratio ) {
	Size imageSize = image.size();
	Size scaledSize( imageSize.width * w_ratio, imageSize.height * h_ratio );

	Mat scalingMatrix = ( Mat_<float>( 2, 2 ) << w_ratio, 0, 0, h_ratio );
	Mat inverseScaling = scalingMatrix.inv();

	return transformation( image, inverseScaling, scaledSize );
}

// Cerca il punto intero più vicino al punto decimale passato come argomento tenendo conto dei boundaries della matrice. Si estrae e analizza la parte decimale per determinare se approssimare per eccesso o per difetto le due coordinate. Restituisce un puntatore a vettore contenente le coordinate intere del punto o NULL se il punto non è compreso nei boundaries specificati (0 e imsize)
int* nearest_neighbour( float* point, Size imsize ) {
	int* int_point = (int*)malloc( sizeof( int ) * 2 );

	int_point[0] = (int)point[0];

	if( point[0] - int_point[0]  > 0.5 )
		int_point[0] += 1;

	int_point[1] = (int)point[1];

	if( point[1] - int_point[1] > 0.5 )
		int_point[1] += 1;

	if( int_point[0] < 0 || int_point[1] < 0 || int_point[0] >= imsize.height || int_point[1] >= imsize.width )
		return NULL;

	return int_point;
}

// Restituisce un vettore con x,y date i,j e la dimensione dell'immagine
float* to_euclid( float i, float j, Size imsize ) {
	float* coordinates = (float*)malloc( sizeof( float ) * 2 );
	coordinates[0] = j - ( imsize.width - 1 ) / 2;
	coordinates[1] = -i + ( imsize.height - 1 ) / 2;

	return coordinates;
}

// Restituisce un vettore con i,j date x,y e la dimensione dell'immagine
float* to_matrix( float x, float y, Size imsize ) {
	float* coordinates = (float*)malloc( sizeof( float ) * 2 );

	coordinates[0] = -y + ( imsize.height - 1 ) / 2;
	coordinates[1] = x + ( imsize.width - 1 ) / 2;

	return coordinates;
}

#endif
